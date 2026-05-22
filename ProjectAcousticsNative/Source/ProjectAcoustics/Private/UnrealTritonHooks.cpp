// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "UnrealTritonHooks.h"
#include "IAcoustics.h"
#include "Async/Async.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"

DEFINE_STAT(STAT_Acoustics_Memory);
DEFINE_STAT(STAT_Acoustics_FileReads);

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LOG HOOK
/////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace TritonRuntime
{
    FTritonLogHook::~FTritonLogHook()
    {
    }

    void FTritonLogHook::Log(ITritonLogHook::LogType type, const char* categoryName, const char* message)
    {
        FString tcharMessage(ANSI_TO_TCHAR(message));
        FString tcharCategory(ANSI_TO_TCHAR(categoryName));
        switch (type)
        {
            case ITritonLogHook::General:
                // Capture the version string here
                {
                    if (tcharMessage.StartsWith(TEXT("TritonRuntime Decoder")))
                    {
                        m_CapturedVersionString = tcharMessage;
                    }

                    UE_LOG(LogAcousticsRuntime, Log, TEXT("[%s] %s"), *tcharCategory, *tcharMessage);
                    break;
                }
            case ITritonLogHook::Warning:
            {
                UE_LOG(LogAcousticsRuntime, Warning, TEXT("[%s] %s"), *tcharCategory, *tcharMessage);
                break;
            }
            // Passing Fatal will trigger a debug breakpoint in the game which we don't want
            // Treat fatal and error the same
            case ITritonLogHook::Fatal:
            case ITritonLogHook::Error:
            {
                UE_LOG(LogAcousticsRuntime, Error, TEXT("[%s] %s"), *tcharCategory, *tcharMessage);
                break;
            }
            default:
            {
                break;
            }
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MEM HOOK
    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOTE: All the memory hook functions below must be thread-safe

    inline void* FTritonMemHook::Malloc(size_t inSize)
    {
        void* outPtr = FMemory::Malloc(inSize, 16);

#if !UE_BUILD_SHIPPING
        // Allocated block size can be larger than requested, get the actual size.
        // (Note that this deals with nullptr correctly)
        auto allocSize = FMemory::GetAllocSize(outPtr);
        INC_MEMORY_STAT_BY(STAT_Acoustics_Memory, allocSize);
        FPlatformAtomics::InterlockedAdd(&m_TotalMemoryUsed, static_cast<int64>(allocSize));
#endif //! UE_BUILD_SHIPPING

        return outPtr;
    }

    void* FTritonMemHook::Realloc(void* inPtr, size_t size)
    {
#if !UE_BUILD_SHIPPING
        int64 oldSize = FMemory::GetAllocSize(inPtr);
#endif

        void* outPtr = FMemory::Realloc(inPtr, size, 16);

#if !UE_BUILD_SHIPPING
        int64 newSize = FMemory::GetAllocSize(outPtr);

        // Increment counters if new size is larger, decrement otherwise
        if (newSize > oldSize)
        {
            INC_MEMORY_STAT_BY(STAT_Acoustics_Memory, newSize - oldSize);
            FPlatformAtomics::InterlockedAdd(&m_TotalMemoryUsed, static_cast<int64>(newSize - oldSize));
        }
        else
        {
            DEC_MEMORY_STAT_BY(STAT_Acoustics_Memory, oldSize - newSize);
            FPlatformAtomics::InterlockedAdd(&m_TotalMemoryUsed, -static_cast<int64>(oldSize - newSize));
        }
#endif //! UE_BUILD_SHIPPING

        return outPtr;
    }

    void FTritonMemHook::Free(void* inPtr)
    {
#if !UE_BUILD_SHIPPING
        // note that this deals will nullptr correctly
        auto AllocSize = FMemory::GetAllocSize(inPtr);
        DEC_MEMORY_STAT_BY(STAT_Acoustics_Memory, AllocSize);
        FPlatformAtomics::InterlockedAdd(&m_TotalMemoryUsed, -static_cast<int64>(AllocSize));
#endif //! UE_BUILD_SHIPPING

        FMemory::Free(inPtr);
    }

    FTritonMemHook::FTritonMemHook() : m_TotalMemoryUsed(0)
    {
    }

    int64 FTritonMemHook::GetTotalMemoryUsed() const
    {
        return m_TotalMemoryUsed;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IO HOOK
    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Read block size
    const size_t FTritonUnrealIOHook::m_ReadCacheSize = 4 * 1024 * 1024;

    uint64 FCachedSyncDiskReader::_DiskRead(uint64 fileOffset, void* destBuffer, uint64 bytesToRead)
    {
        check(IsOK());

        auto request = TUniquePtr<IAsyncReadRequest>(m_FileHandle->ReadRequest(fileOffset, bytesToRead, AIOP_Normal));
        if (!request->WaitCompletion())
        {
            // Something went wrong with loading
            return -1;
        }
        auto result = request->GetReadResults();

        if (result == nullptr)
        {
            return 0;
        }

        // Ideally, we'd also call GetReadSize() here, but it never returns a valid size even on successful reads
        FMemory::Memcpy(destBuffer, result, bytesToRead);
        // By contract, we must call free on the result buffer now that we're done with it
        FMemory::Free(result);

#if !UE_BUILD_SHIPPING
        INC_DWORD_STAT_BY(STAT_Acoustics_FileReads, bytesToRead);
        m_BytesRead += static_cast<int64>(bytesToRead);
#endif

        return bytesToRead;
    }

    FCachedSyncDiskReader::FCachedSyncDiskReader(const FString& fileName, uint64 cacheSize)
        : m_FileName(fileName), m_CacheSize(cacheSize), m_CacheFileOffset(0), m_IsCacheInvalid(true), m_BytesRead(0)
    {
        m_CacheData.Reserve(cacheSize);

        m_FileSize = IFileManager::Get().FileSize(*fileName);
        // If there were any errors, such as file not found, m_FileSize will be -1
        if (m_FileSize != -1)
        {
            m_FileHandle.Reset(FPlatformFileManager::Get().GetPlatformFile().OpenAsyncRead(*m_FileName));
        }
    }

    bool FCachedSyncDiskReader::IsOK() const
    {
        return m_FileSize != -1;
    }

    FCachedSyncDiskReader::~FCachedSyncDiskReader()
    {
#if !UE_BUILD_SHIPPING
        SET_DWORD_STAT(STAT_Acoustics_FileReads, 0);
#endif
    }

    int64 FCachedSyncDiskReader::GetFileSize() const
    {
        return m_FileSize;
    }

    uint64 FCachedSyncDiskReader::Read(uint64 readOffset, void* destBuffer, uint64 bytesToRead)
    {
        check(IsOK());

        if (readOffset + bytesToRead > (uint64) m_FileSize) // Reading past EOF
        {
            return 0;
        }

        if (bytesToRead >= m_CacheSize) // big read, bypass cache
        {
            return _DiskRead(readOffset, destBuffer, bytesToRead);
        }

        bool cacheHit;

        if (m_IsCacheInvalid)
        {
            cacheHit = false;
        }
        else
        {
            cacheHit = (readOffset >= m_CacheFileOffset && readOffset + bytesToRead <= m_CacheFileOffset + m_CacheSize);
        }

        if (cacheHit)
        {
            memcpy(destBuffer, m_CacheData.GetData() + readOffset - m_CacheFileOffset, bytesToRead);
            return bytesToRead;
        }
        else // Cache miss: Update cache and recurse
        {
            uint64 distToFileEnd = m_FileSize - readOffset;
            uint64 diskReadSize = FMath::Min(distToFileEnd, static_cast<uint64>(m_CacheSize));
            uint64 actuallyRead = _DiskRead(readOffset, m_CacheData.GetData(), diskReadSize);
            // Failed read request, cache contents are unknown now, invalidate it
            if (actuallyRead < diskReadSize)
            {
                m_IsCacheInvalid = true;
                return 0;
            }

            m_IsCacheInvalid = false;
            m_CacheFileOffset = readOffset;

            // recurse, this time cache will be hit
            return Read(readOffset, destBuffer, bytesToRead);
        }

        //return -1;
    }

    int64 FCachedSyncDiskReader::GetBytesRead() const
    {
        return m_BytesRead;
    }

    FTritonUnrealIOHook::FTritonUnrealIOHook()
    {
    }

    FTritonUnrealIOHook::~FTritonUnrealIOHook()
    {
        Close();
    }

    bool FTritonUnrealIOHook::OpenForRead(const char* name)
    {
        m_FileOffset = 0;
        m_DiskReader = TUniquePtr<FCachedSyncDiskReader>(new FCachedSyncDiskReader(FString(name), m_ReadCacheSize));
        return m_DiskReader->IsOK();
    }

    size_t FTritonUnrealIOHook::Read(void* destBuffer, size_t elementSize, size_t numElementsToRead)
    {
        uint64 bytesToRead = elementSize * numElementsToRead;
        uint64 bytesActuallyRead = m_DiskReader->Read(m_FileOffset, destBuffer, bytesToRead);

        m_FileOffset += bytesActuallyRead;

        return (bytesActuallyRead / elementSize);
    }

    bool FTritonUnrealIOHook::Seek(uint32_t offset)
    {
        m_FileOffset = static_cast<uint64>(offset);
        return true;
    }

    bool FTritonUnrealIOHook::SeekFromCurrent(uint32_t offset)
    {
        m_FileOffset += static_cast<uint64>(offset);
        return true;
    }

    int64 FTritonUnrealIOHook::GetFileSize() const
    {
        return m_DiskReader->GetFileSize();
    }

    bool FTritonUnrealIOHook::Close()
    {
        m_DiskReader = nullptr;
        m_FileOffset = 0;
        return true;
    }

    int64 FTritonUnrealIOHook::GetBytesRead() const
    {
        return m_DiskReader != nullptr ? m_DiskReader->GetBytesRead() : 0;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TASK HOOK
    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    class FTritonLoadAsyncTask : public IQueuedWork
    {
    public:
        FTritonLoadAsyncTask(TFunction<void()>&& inFunction, volatile int32* inDoneCounter)
            : m_Function(inFunction), m_DoneCounter(inDoneCounter)
        {
        }

        virtual void DoThreadedWork() override
        {
            SCOPED_NAMED_EVENT_TEXT("Triton Streaming", FColor::Green);
            m_Function();
            FPlatformAtomics::InterlockedDecrement(m_DoneCounter);
        }

        /**
         * Tells the queued work that it is being abandoned so that it can do
         * per object clean up as needed. This will only be called if it is being
         * abandoned before completion. NOTE: This requires the object to delete
         * itself using whatever heap it was allocated in.
         */
        virtual void Abandon() override
        {
            // most implementations of IQueuedWork do this to signal completion.
            FPlatformAtomics::InterlockedDecrement(m_DoneCounter);
        }

        /** The function to execute on the Task Graph. */
        TFunction<void()> m_Function;

        volatile int32* m_DoneCounter;
    };

    FTritonAsyncTaskHook::FTritonAsyncTaskHook() : m_NumRunningTasks(0)
    {
    }

    FTritonAsyncTaskHook::~FTritonAsyncTaskHook()
    {
    }

    void FTritonAsyncTaskHook::Launch(const TaskFunc* task)
    {
        // Make a local deep copy of Task, as the object has no existence guarantee beyond this call
        m_TaskFunc.Reset(task->Clone());
        TFunction<void()> Function([this]() { m_TaskFunc->Execute(); });

        FPlatformAtomics::InterlockedIncrement(&m_NumRunningTasks);

        GThreadPool->AddQueuedWork(new FTritonLoadAsyncTask(MoveTemp(Function), &m_NumRunningTasks));
    }

    void FTritonAsyncTaskHook::Wait()
    {
        // Busy loop. Called only during map unload when doing non-blocking streaming.
        while (m_NumRunningTasks > 0)
        {
            FPlatformProcess::Sleep(0);
        }
    }

    // Used by triton to synchronize and update internal work queue shared with async task
    void FTritonAsyncTaskHook::Lock()
    {
        m_Lock.Lock();
    }

    void FTritonAsyncTaskHook::Unlock()
    {
        m_Lock.Unlock();
    }
} // namespace TritonRuntime
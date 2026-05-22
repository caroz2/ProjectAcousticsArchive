// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "TritonHooks.h"
#include "Async/AsyncFileHandle.h"
#include "Stats/Stats2.h"
#include "IAcoustics.h"

namespace TritonRuntime
{
    // Implements the Interface for logging Triton's internal debug messages
    // Will route all debug messages to the UE output window
    class FTritonLogHook : public ITritonLogHook
    {
    public:
        FString m_CapturedVersionString;
        virtual ~FTritonLogHook();
        virtual void Log(ITritonLogHook::LogType type, const char* categoryName, const char* message) override;
    };

    // Implements the interface for memory alloc/dealloc operations. All operations *must* be thread-safe.
    // Routes all of Triton's internal new/deletes to UE's FMemory::* versions.
    class FTritonMemHook : public ITritonMemHook
    {
        volatile int64 m_TotalMemoryUsed;
        virtual void* Malloc(size_t inSize);
        virtual void* Realloc(void* inPtr, size_t size);
        virtual void Free(void* inPtr);

    public:
        FTritonMemHook();
        int64 GetTotalMemoryUsed() const;
    };

    // Handles file I/O for UFS.
    class FCachedSyncDiskReader
    {
    private:
        FString m_FileName;
        TUniquePtr<IAsyncReadFileHandle> m_FileHandle;

        TArray<unsigned char> m_CacheData;
        size_t m_CacheSize;
        size_t m_CacheFileOffset;
        bool m_IsCacheInvalid;

        int64 m_FileSize;
        uint64 _DiskRead(uint64 fileOffset, void* destBuffer, uint64 bytesToRead);

        volatile int64 m_BytesRead;

    public:
        FCachedSyncDiskReader(const FString& fileName, uint64 cacheSize);
        virtual ~FCachedSyncDiskReader();
        bool IsOK() const;
        int64 GetFileSize() const;
        uint64 Read(uint64 readOffset, void* destBuffer, uint64 bytesToRead);
        int64 GetBytesRead() const;
    };

    // Implements Triton's Interface for blocking I/O from a single file/asset. Operations need not be thread-safe.
    // Allows ACE files to be retrieved from PAK files
    class FTritonUnrealIOHook : public ITritonIOHook
    {
    private:
        uint64 m_FileOffset;
        static const size_t m_ReadCacheSize;
        TUniquePtr<FCachedSyncDiskReader> m_DiskReader;

    public:
        FTritonUnrealIOHook();
        virtual ~FTritonUnrealIOHook();
        virtual bool OpenForRead(const char* name) override;
        virtual size_t Read(void* destBuffer, size_t elementSize, size_t numElementsToRead) override;
        virtual bool Seek(uint32_t offset) override;
        virtual bool SeekFromCurrent(uint32_t offset) override;
        virtual bool Close() override;
        int64 GetFileSize() const;
        int64 GetBytesRead() const;
    };

    // Implements Triton's Interface for launching an asynchronous task.
    // Queues tasks onto UE's GThreadPool
    class FTritonAsyncTaskHook : public ITritonAsyncTaskHook
    {
    private:
        TUniquePtr<TaskFunc> m_TaskFunc;
        FGraphEventRef m_TaskRef;
        FCriticalSection m_Lock;
        volatile int32 m_NumRunningTasks;

    public:
        FTritonAsyncTaskHook();
        virtual ~FTritonAsyncTaskHook();
        virtual void Launch(const TaskFunc* task) override;
        virtual void Wait() override;
        virtual void Lock() override;
        virtual void Unlock() override;
    };
} // namespace TritonRuntime

DECLARE_MEMORY_STAT_EXTERN(TEXT("Acoustics Memory Usage"), STAT_Acoustics_Memory, STATGROUP_Acoustics, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(
    TEXT("Acoustics Total Bytes Read"), STAT_Acoustics_FileReads, STATGROUP_Acoustics, );
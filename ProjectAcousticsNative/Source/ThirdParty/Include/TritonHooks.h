// Copyright (c) 2022 http://www.microsoft.com. All rights reserved.
//
// Declares various Triton hook interfaces
// Nikunjr, 7/15/2013
#pragma once
#include <cstddef>
#include <cstdint>

namespace TritonRuntime
{
    /// <summary>	Interface for blocking i/o from a single file/asset. Operations need not be thread-safe. </summary>
    class ITritonIOHook
    {
    public:
        virtual ~ITritonIOHook(){};
        virtual bool OpenForRead(const char* Name) = 0;
        virtual size_t Read(void* DestBuffer, size_t ElementSize, size_t NumElementsToRead) = 0;
        virtual bool SeekFromCurrent(uint32_t offset) = 0;
        virtual bool Seek(uint32_t offset) = 0;
        virtual bool Close() = 0;
    };

    /// <summary> Interface for memory alloc/dealloc operations. All operations *must* be thread-safe. </summary>
    class ITritonMemHook
    {
    public:
        virtual ~ITritonMemHook(){};
        virtual void* Malloc(size_t Size) = 0;
        virtual void* Realloc(void* ptr, size_t Size) = 0;
        virtual void Free(void* ptr) = 0;

        virtual void StartAllocationScope(const wchar_t* ScopeName)
        {
            ScopeName = nullptr;
        } // avoid C4100

        virtual void StopAllocationScope(const wchar_t* ScopeName)
        {
            ScopeName = nullptr;
        } // avoid C4100
    };

    // Implements interface similar to std::function<void(void)> without STL dependency
    class TaskFunc
    {
    public:
        virtual ~TaskFunc()
        {
        }
        virtual void Execute() const = 0;
        // Caller owns and is responsible for deleting returned object
        virtual TaskFunc* Clone() const = 0;
    };

    /// <summary> Interface for launching an asynchronous task. </summary>
    class ITritonAsyncTaskHook
    {
    public:
        virtual ~ITritonAsyncTaskHook(){};
        /// <summary>	Run an asynchronous task  </summary>
        ///
        /// <param name="Task"> The work to perform.
        /// </param>
        /// <remarks>
        ///
        /// Note that there is no lifetime guarantee of the backing object for
        /// the input Task object beyond function call, if needed,
        /// make a local copy via Clone() that you are responsible for cleaning up.
        ///
        /// If you have a general thread pool system that should map pretty
        /// easily to this hook.
        /// However, a simple worker thread implementation
        /// that serves Launch() calls in sequence will also work nicely.
        /// Launch calls will come in sequentially in this sense --
        /// A call to Launch() will occur only if a prior Launch()
        /// call has finished its work and is not holding onto the lock
        /// (obtained via Lock()). Overlap between Launch calls is thus
        /// restricted purely to how much time task cleanup takes on your
        /// implementation. Blocking for previous Launch() to finish will
        /// ensure strict sequence and will generally only block for the
        /// duration of your task cleanup, which is generally small.
        ///
        /// </remarks>

        virtual void Launch(const TaskFunc* Task) = 0;
        // This function should wait on async task to finish before returning
        virtual void Wait() = 0;
        // The async task above and main thread calling Triton APIs
        // will synchronize using these functions
        virtual void Lock() = 0;
        virtual void Unlock() = 0;
    };

    /// <summary> Interface for logging Triton's internal debug messages. </summary>
    class ITritonLogHook
    {
    public:
        enum LogType
        {
            General,
            Warning,
            Error,
            Fatal
        };
        virtual ~ITritonLogHook(){};
        virtual void Log(LogType type, const char* CategoryName, const char* Message) = 0;
    };
} // namespace TritonRuntime

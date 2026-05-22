// Copyright (c) 2022 http://www.microsoft.com. All rights reserved.
//
// Declares shared stuff for all internal classes -- never include this in a public header
// Nikunjr, 9/11/2013

#pragma once

#ifdef USE_MEMORY_HOOK

#include "TritonHooks.h"

namespace TritonRuntime
{
    extern ITritonMemHook* GetMemHook();
}

#define DEFINE_TRITON_CUSTOM_ALLOCATORS                                                                                \
public:                                                                                                                \
    static void* operator new(size_t size)                                                                             \
    {                                                                                                                  \
        return TritonRuntime::GetMemHook()->Malloc(size);                                                              \
    }                                                                                                                  \
    static void operator delete(void* ptr)                                                                             \
    {                                                                                                                  \
        TritonRuntime::GetMemHook()->Free(ptr);                                                                        \
    }                                                                                                                  \
    static void* operator new[](size_t size)                                                                           \
    {                                                                                                                  \
        return TritonRuntime::GetMemHook()->Malloc(size);                                                              \
    }                                                                                                                  \
    static void operator delete[](void* ptr)                                                                           \
    {                                                                                                                  \
        TritonRuntime::GetMemHook()->Free(ptr);                                                                        \
    }                                                                                                                  \
    static void* operator new(size_t, void* ptr)                                                                       \
    {                                                                                                                  \
        return ptr;                                                                                                    \
    }                                                                                                                  \
    static void operator delete(void*, void*)                                                                          \
    {                                                                                                                  \
    }                                                                                                                  \
    static void* operator new[](size_t, void* ptr)                                                                     \
    {                                                                                                                  \
        return ptr;                                                                                                    \
    }                                                                                                                  \
    static void operator delete[](void*, void*)                                                                        \
    {                                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
private:

#define TRITON_PREVENT_HEAP_ALLOCATION                                                                                 \
private:                                                                                                               \
    void* operator new(size_t);                                                                                        \
    void* operator new[](size_t);                                                                                      \
    void operator delete(void*);                                                                                       \
    void operator delete[](void*);

#else // USE_MEMORY_HOOK
// Do not do any custom allocators when compiling into precomputation
#define DEFINE_TRITON_CUSTOM_ALLOCATORS
#define TRITON_PREVENT_HEAP_ALLOCATION
#endif // USE_MEMORY_HOOK
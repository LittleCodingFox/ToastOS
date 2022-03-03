#include <stdint.h>
#include <stddef.h>
#include <liballoc.h>
#include <debug.hpp>
#include "paging/PageFrameAllocator.hpp"
#include "paging/PageTableManager.hpp"
#include "stacktrace/stacktrace.hpp"

AtomicLock lock;

extern "C" int liballoc_lock()
{
    lock.Lock();

    return 0;
}

extern "C" int liballoc_unlock()
{
    lock.Unlock();

    return 0;
}

extern "C" void* liballoc_alloc(size_t pages)
{
    void *ptr = globalAllocator.RequestPages(pages);

    if(ptr == NULL)
    {
        DEBUG_OUT("alloc: Failed to request %llu pages", pages);

        KernelDumpStacktrace();

        return NULL;
    }

    void *realPtr = (void *)TranslateToHighHalfMemoryAddress((uint64_t)ptr);

    //DEBUG_OUT("alloc: allocated %i pages at address %p", pages, realPtr);

    return realPtr;
}

extern "C" int liballoc_free(void* ptr, size_t pages)
{
    void *realPtr = (void *)TranslateToPhysicalMemoryAddress((uint64_t)ptr);
    
    globalAllocator.FreePages(realPtr, pages);

    //DEBUG_OUT("alloc: freed %i pages at address %p", pages, ptr);

    return 0;
}

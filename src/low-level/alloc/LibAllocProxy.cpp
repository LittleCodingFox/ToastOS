#include <stdint.h>
#include <stddef.h>
#include <liballoc.h>
#include <debug.hpp>
#include "paging/PageFrameAllocator.hpp"
#include "paging/PageTableManager.hpp"
#include "threading/lock.hpp"
#include "registers/Registers.hpp"
#include <string.h>

AtomicLock liballocLock;

extern "C" int liballoc_lock()
{
    liballocLock.Lock();

    return 0;
}

extern "C" int liballoc_unlock()
{
    liballocLock.Unlock();

    return 0;
}

extern "C" void* liballoc_alloc(size_t pages)
{
    void *ptr = globalAllocator.RequestPages(pages);

    if(ptr == NULL)
    {
        return NULL;
    }

    void *realPtr = (void *)TranslateToHighHalfMemoryAddress((uint64_t)ptr);

    DEBUG_OUT("alloc: allocated %i pages at address %p", pages, realPtr);

    return realPtr;
}

extern "C" int liballoc_free(void* ptr, size_t pages)
{
    void *realPtr = (void *)TranslateToPhysicalMemoryAddress((uint64_t)ptr);
    
    globalAllocator.FreePages(realPtr, pages);

    DEBUG_OUT("free: freed %i pages at address %p", pages, ptr);

    return 0;
}

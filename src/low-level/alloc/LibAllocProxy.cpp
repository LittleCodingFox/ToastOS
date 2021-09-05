#include <stdint.h>
#include <stddef.h>
#include <liballoc.h>
#include <debug.hpp>
#include "paging/PageFrameAllocator.hpp"
#include "paging/PageTableManager.hpp"

int liballoc_lock()
{
    return 0;
}

int liballoc_unlock()
{
    return 0;
}

void* liballoc_alloc(size_t pages)
{
    void *ptr = globalAllocator.RequestPages(pages);

    if(ptr == NULL)
    {
        return NULL;
    }

    for(uint32_t i = 0; i < pages; i++)
    {
        globalPageTableManager->MapMemory((void*)TranslateToHighHalfMemoryAddress((uint64_t)ptr + i * 0x1000), (void*)((uint64_t)ptr + i * 0x1000),
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
    }

    void *realPtr = (void *)TranslateToHighHalfMemoryAddress((uint64_t)ptr);

    DEBUG_OUT("liballoc_alloc: allocated %i pages at address %p", pages, realPtr);

    return realPtr;
}

int liballoc_free(void* ptr, size_t pages)
{
    void *realPtr = (void *)TranslateToPhysicalMemoryAddress((uint64_t)ptr);
    globalAllocator.FreePages(realPtr, pages);

    DEBUG_OUT("liballoc_free: freed %i pages at address %p", pages, realPtr);

    for(uint32_t i = 0; i < pages; i++)
    {
        globalPageTableManager->UnmapMemory((void*)((uint64_t)ptr + i * 0x1000));
    }

    return 0;
}

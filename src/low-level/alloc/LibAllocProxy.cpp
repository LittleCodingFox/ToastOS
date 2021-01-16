#include <stdint.h>
#include <stddef.h>
#include <liballoc.h>
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
    void *ptr = GlobalAllocator.RequestPages(pages);

    if(ptr == NULL)
    {
        return NULL;
    }

    for(uint32_t i = 0; i < pages; i++)
    {
        GlobalPageTableManager->MapMemory((void*)((uint64_t)ptr + i * 4096), (void*)((uint64_t)ptr + i * 4096));
    }

    return ptr;
}

int liballoc_free(void* ptr, size_t pages)
{
    GlobalAllocator.FreePages(ptr, pages);

    for(uint32_t i = 0; i < pages; i++)
    {
        GlobalPageTableManager->UnmapMemory((void*)((uint64_t)ptr + i * 4096));
    }

    return 0;
}

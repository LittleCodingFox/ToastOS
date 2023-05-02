#include <stdint.h>
#include <stddef.h>
#include "dlmalloc/dlmalloc.h"
#include <debug.hpp>
#include "paging/PageFrameAllocator.hpp"
#include "paging/PageTableManager.hpp"
#include "threading/lock.hpp"
#include "registers/Registers.hpp"
#include <string.h>

AtomicLock allocLock;

static uintptr_t heapAddress = 0;

extern "C" void* libk_sbrk(ptrdiff_t size)
{
    ScopedLock lock(allocLock);

    if(size == 0)
    {
        return (void *)heapAddress;
    }

    auto pageCount = size / 0x1000 + (size % 0x1000 ? 1 : 0);

    auto pages = globalAllocator.RequestPages(pageCount);

    heapAddress = TranslateToHighHalfMemoryAddress((uint64_t)pages);

    for(auto i = 0; i < pageCount; i++)
    {
        globalPageTableManager->MapMemory((void *)((uint64_t)heapAddress + i * 0x1000), (void *)((uint64_t)pages + i * 0x1000), PAGING_FLAG_WRITABLE);
    }

    return (void *)heapAddress;
}

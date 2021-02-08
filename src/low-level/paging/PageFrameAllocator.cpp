#include "PageFrameAllocator.hpp"
#include "debug.hpp"
#include "Panic.hpp"
#include <string.h>
#include "stacktrace/stacktrace.hpp"

uint64_t freeMemory;
uint64_t reservedMemory;
uint64_t usedMemory;
bool Initialized = false;
PageFrameAllocator globalAllocator;

void PageFrameAllocator::readEFIMemoryMap(EFI_MEMORY_DESCRIPTOR* mMap, size_t mMapSize, size_t mMapDescSize)
{
    if (Initialized) return;

    Initialized = true;

    uint64_t mMapEntries = mMapSize / mMapDescSize;

    void* largestFreeMemSeg = NULL;
    size_t largestFreeMemSegSize = 0;

    for (int i = 0; i < mMapEntries; i++)
    {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)mMap + (i * mMapDescSize));

        DEBUG_OUT("EFI MMap Entry %i: %u type (%s), size %llu, addr: %p", i, desc->type,
            EFI_MEMORY_TYPE_STRINGS[desc->type], desc->numPages * 4096, desc->physAddr);

        if (desc->type == 7) // type = EfiConventionalMemory
        {
            if (desc->numPages * 4096 > largestFreeMemSegSize)
            {
                largestFreeMemSeg = (void*)desc->physAddr;
                largestFreeMemSegSize = desc->numPages * 4096;
            }
        }
    }

    uint64_t memorySize = getMemorySize(mMap, mMapEntries, mMapDescSize);
    freeMemory = memorySize;

    uint64_t bitmapSize = memorySize / 4096 / 8 + 1;

    initBitmap(bitmapSize, largestFreeMemSeg);

    for (int i = 0; i < mMapEntries; i++)
    {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)mMap + (i * mMapDescSize));

        if (desc->type == 7)
        {
            for(uint64_t index = 0, startIndex = desc->physAddr / 4096; index < desc->numPages; index++)
            {
                PageBitmap.Set(index + startIndex, false);

                freeMemory += 4096;
                reservedMemory -= 4096;
            }
        }
    }

    lockPages(PageBitmap.buffer, PageBitmap.size / 4096 + 1);

    DEBUG_OUT("%s", "Finished setting up EFI memory");
}

void PageFrameAllocator::initBitmap(size_t bitmapSize, void* bufferAddress)
{
    PageBitmap.size = bitmapSize;
    PageBitmap.buffer = (uint8_t*)bufferAddress;

    DEBUG_OUT("Initing bitmap with size %d at address %p", bitmapSize, bufferAddress);

    memset(PageBitmap.buffer, 0, bitmapSize);

    //Set everything as locked due to gaps between memory, also takes care of reserved memory at the same time
    for(uint64_t i = 0; i < PageBitmap.size * 8; i++)
    {
        PageBitmap.Set(i, true);
        
        freeMemory -= 4096;
        reservedMemory += 4096;
    }
}

void* PageFrameAllocator::requestPage()
{
    for (uint64_t i = 0; i < PageBitmap.size * 8; i++)
    {
        if (PageBitmap[i] == true) continue;

        lockPage((void*)(i * 4096));

        return (void*)(i * 4096);
    }

    return NULL; // Page Frame Swap to file
}

void *PageFrameAllocator::requestPages(uint32_t count)
{
    uint32_t freeCount = 0;

    for (uint32_t i = 0; i < PageBitmap.size * 8; i++)
    {
        if (PageBitmap[i] == false)
        {
            freeCount++;

            if(freeCount == count)
            {
                uint32_t index = 1 + i - count;

                lockPages((void*)((uint64_t)index * 4096), count);

                return (void *)((uint64_t)index * 4096);
            }
        }
        else
        {
            freeCount = 0;
        }
    }

    return NULL;
}

void PageFrameAllocator::freePage(void* address)
{
    uint64_t index = (uint64_t)address / 4096;

    if (PageBitmap[index] == false)
    {
        return;
    }

    if (PageBitmap.Set(index, false))
    {
        freeMemory += 4096;
        usedMemory -= 4096;
    }
}

void PageFrameAllocator::freePages(void* address, uint64_t pageCount)
{
    for (int t = 0; t < pageCount; t++)
    {
        freePage((void*)((uint64_t)address + (t * 4096)));
    }
}

void PageFrameAllocator::lockPage(void* address)
{
    uint64_t index = (uint64_t)address / 4096;

    if (PageBitmap[index] == true)
    {
        return;
    }

    if (PageBitmap.Set(index, true))
    {
        freeMemory -= 4096;
        usedMemory += 4096;
    }
}

void PageFrameAllocator::lockPages(void* address, uint64_t pageCount)
{
    for (int t = 0; t < pageCount; t++)
    {
        lockPage((void*)((uint64_t)address + (t * 4096)));
    }
}

void PageFrameAllocator::unreservePage(void* address)
{
    uint64_t index = (uint64_t)address / 4096;

    if (PageBitmap[index] == false) return;

    if (PageBitmap.Set(index, false))
    {
        freeMemory += 4096;
        reservedMemory -= 4096;
    }
}

void PageFrameAllocator::unreservePages(void* address, uint64_t pageCount)
{
    for (int t = 0; t < pageCount; t++)
    {
        unreservePage((void*)((uint64_t)address + (t * 4096)));
    }
}

void PageFrameAllocator::reservePage(void* address)
{
    uint64_t index = (uint64_t)address / 4096;

    if (PageBitmap[index] == true) return;

    if (PageBitmap.Set(index, true))
    {
        freeMemory -= 4096;
        reservedMemory += 4096;
    }
}

void PageFrameAllocator::reservePages(void* address, uint64_t pageCount)
{
    for (int t = 0; t < pageCount; t++)
    {
        reservePage((void*)((uint64_t)address + (t * 4096)));
    }
}

uint64_t PageFrameAllocator::getFreeRAM()
{
    return freeMemory;
}

uint64_t PageFrameAllocator::getUsedRAM()
{
    return usedMemory;
}

uint64_t PageFrameAllocator::getReservedRAM()
{
    return reservedMemory;
}

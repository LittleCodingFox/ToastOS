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

void PageFrameAllocator::ReadEFIMemoryMap(EFI_MEMORY_DESCRIPTOR* mMap, size_t mMapSize, size_t mMapDescSize)
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

    uint64_t memorySize = GetMemorySize(mMap, mMapEntries, mMapDescSize);
    freeMemory = memorySize;

    uint64_t bitmapSize = memorySize / 4096 / 8 + 1;

    InitBitmap(bitmapSize, largestFreeMemSeg);

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

    LockPages(PageBitmap.buffer, PageBitmap.size / 4096 + 1);

    DEBUG_OUT("%s", "Finished setting up EFI memory");
}

void PageFrameAllocator::InitBitmap(size_t bitmapSize, void* bufferAddress)
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

void* PageFrameAllocator::RequestPage()
{
    for (uint64_t i = 0; i < PageBitmap.size * 8; i++)
    {
        if (PageBitmap[i] == true) continue;

        LockPage((void*)(i * 4096));

        return (void*)(i * 4096);
    }

    return NULL; // Page Frame Swap to file
}

void *PageFrameAllocator::RequestPages(uint32_t count)
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

                LockPages((void*)((uint64_t)index * 4096), count);

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

void PageFrameAllocator::FreePage(void* address)
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

void PageFrameAllocator::FreePages(void* address, uint64_t pageCount)
{
    for (int t = 0; t < pageCount; t++)
    {
        FreePage((void*)((uint64_t)address + (t * 4096)));
    }
}

void PageFrameAllocator::LockPage(void* address)
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

void PageFrameAllocator::LockPages(void* address, uint64_t pageCount)
{
    for (int t = 0; t < pageCount; t++)
    {
        LockPage((void*)((uint64_t)address + (t * 4096)));
    }
}

void PageFrameAllocator::UnreservePage(void* address)
{
    uint64_t index = (uint64_t)address / 4096;

    if (PageBitmap[index] == false) return;

    if (PageBitmap.Set(index, false))
    {
        freeMemory += 4096;
        reservedMemory -= 4096;
    }
}

void PageFrameAllocator::UnreservePages(void* address, uint64_t pageCount)
{
    for (int t = 0; t < pageCount; t++)
    {
        UnreservePage((void*)((uint64_t)address + (t * 4096)));
    }
}

void PageFrameAllocator::ReservePage(void* address)
{
    uint64_t index = (uint64_t)address / 4096;

    if (PageBitmap[index] == true) return;

    if (PageBitmap.Set(index, true))
    {
        freeMemory -= 4096;
        reservedMemory += 4096;
    }
}

void PageFrameAllocator::ReservePages(void* address, uint64_t pageCount)
{
    for (int t = 0; t < pageCount; t++)
    {
        ReservePage((void*)((uint64_t)address + (t * 4096)));
    }
}

uint64_t PageFrameAllocator::GetFreeRAM()
{
    return freeMemory;
}

uint64_t PageFrameAllocator::GetUsedRAM()
{
    return usedMemory;
}

uint64_t PageFrameAllocator::GetReservedRAM()
{
    return reservedMemory;
}

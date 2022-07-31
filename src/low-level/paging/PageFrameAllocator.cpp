#include "PageFrameAllocator.hpp"
#include "debug.hpp"
#include "Panic.hpp"
#include "KernelUtils.hpp"
#include <string.h>
#include "stacktrace/stacktrace.hpp"

uint64_t freeMemory = 0;
uint64_t reservedMemory = 0;
uint64_t usedMemory = 0;
bool Initialized = false;
PageFrameAllocator globalAllocator;

const char *MemoryMapTypeString(int type)
{
    switch(type)
    {
        case LIMINE_MEMMAP_USABLE:

            return "Usable"; 

        case LIMINE_MEMMAP_RESERVED:

            return "Reserved";

        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:

            return "ACPI Reclaimable";

        case LIMINE_MEMMAP_ACPI_NVS:

            return "ACPI NVS";

        case LIMINE_MEMMAP_BAD_MEMORY:

            return "Bad Memory";

        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:

            return "Bootloader Reclaimable";

        case LIMINE_MEMMAP_KERNEL_AND_MODULES:

            return "Kernel and Modules";

        case LIMINE_MEMMAP_FRAMEBUFFER:

            return "Framebuffer";

        default:

            return "Unknown";
    }
}

void PageFrameAllocator::ReadMemoryMap(volatile limine_memmap_request* memmap)
{
    if (Initialized) return;

    Initialized = true;

    void* largestFreeMemSeg = NULL;
    size_t largestFreeMemSegSize = 0;

    for (uint64_t i = 0; i < memmap->response->entry_count; i++)
    {
        limine_memmap_entry* desc = (limine_memmap_entry*)memmap->response->entries[i];

        DEBUG_OUT("MMap Entry %i: type: %s, size %llu, addr: %p", i, MemoryMapTypeString(desc->type),
            desc->length, desc->base);

        if (desc->type == LIMINE_MEMMAP_USABLE)
        {
            if (desc->length > largestFreeMemSegSize)
            {
                largestFreeMemSeg = (void*)desc->base;
                largestFreeMemSegSize = desc->length;
            }
        }
    }

    uint64_t memorySize = GetMemorySize(memmap);
    freeMemory = memorySize;

    uint64_t bitmapSize = memorySize / 0x1000 / 8 + 1;

    InitBitmap(bitmapSize, largestFreeMemSeg);

    DEBUG_OUT("Memory Size: %llu", GetMemorySize(memmap));

    for (uint64_t i = 0; i < memmap->response->entry_count; i++)
    {
        limine_memmap_entry* desc = (limine_memmap_entry*)memmap->response->entries[i];

        if (desc->type == LIMINE_MEMMAP_USABLE)
        {
            DEBUG_OUT("Unlocking usable pages for %p-%p (%llu pages)", desc->base, desc->base + desc->length, desc->length / 0x1000);

            for(uint64_t index = 0, startIndex = desc->base / 0x1000; index < desc->length / 0x1000; index++)
            {
                PageBitmap.Set(index + startIndex, false);

                freeMemory += 0x1000;
                reservedMemory -= 0x1000;
            }
        }
    }

    LockPages(PageBitmap.buffer, PageBitmap.size / 0x1000 + 1);

    DEBUG_OUT("%s", "Finished setting up memory");
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
        
        freeMemory -= 0x1000;
        reservedMemory += 0x1000;
    }

    DEBUG_OUT("%s", "Bitmap initialized!");
}

void* PageFrameAllocator::RequestPage()
{
    for (uint64_t i = 0; i < PageBitmap.size * 8; i++)
    {
        if (PageBitmap[i] == true) continue;

        LockPage((void*)(i * 0x1000));

        return (void*)(i * 0x1000);
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
            if(freeCount == count)
            {
                uint32_t index = i - count;

                LockPages((void*)((uint64_t)index * 0x1000), count);

                return (void *)((uint64_t)index * 0x1000);
            }

            freeCount++;
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
    uint64_t index = (uint64_t)address / 0x1000;

    if (PageBitmap[index] == false)
    {
        return;
    }

    if (PageBitmap.Set(index, false))
    {
        freeMemory += 0x1000;
        usedMemory -= 0x1000;
    }
}

void PageFrameAllocator::FreePages(void* address, uint64_t pageCount)
{
    for (uint64_t t = 0; t < pageCount; t++)
    {
        FreePage((void*)((uint64_t)address + (t * 0x1000)));
    }
}

void PageFrameAllocator::LockPage(void* address)
{
    uint64_t index = (uint64_t)address / 0x1000;

    if (PageBitmap[index] == true)
    {
        return;
    }

    if (PageBitmap.Set(index, true))
    {
        freeMemory -= 0x1000;
        usedMemory += 0x1000;
    }
}

void PageFrameAllocator::LockPages(void* address, uint64_t pageCount)
{
    for (uint64_t t = 0; t < pageCount; t++)
    {
        LockPage((void*)((uint64_t)address + (t * 0x1000)));
    }
}

void PageFrameAllocator::UnreservePage(void* address)
{
    uint64_t index = (uint64_t)address / 0x1000;

    if (PageBitmap[index] == false) return;

    if (PageBitmap.Set(index, false))
    {
        freeMemory += 0x1000;
        reservedMemory -= 0x1000;
    }
}

void PageFrameAllocator::UnreservePages(void* address, uint64_t pageCount)
{
    for (uint64_t t = 0; t < pageCount; t++)
    {
        UnreservePage((void*)((uint64_t)address + (t * 0x1000)));
    }
}

void PageFrameAllocator::ReservePage(void* address)
{
    uint64_t index = (uint64_t)address / 0x1000;

    if (PageBitmap[index] == true) return;

    if (PageBitmap.Set(index, true))
    {
        freeMemory -= 0x1000;
        reservedMemory += 0x1000;
    }
}

void PageFrameAllocator::ReservePages(void* address, uint64_t pageCount)
{
    for (uint64_t t = 0; t < pageCount; t++)
    {
        ReservePage((void*)((uint64_t)address + (t * 0x1000)));
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

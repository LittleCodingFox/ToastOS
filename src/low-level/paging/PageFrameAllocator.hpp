#pragma once
#include <limine.h>
#include <stdint.h>
#include "Bitmap.hpp"

class PageFrameAllocator
{
public:
    Bitmap PageBitmap;

    void ReadMemoryMap(volatile limine_memmap_request* memmap, volatile limine_hhdm_request *hhd);
    void FreePage(void* address);
    void FreePages(void* address, uint64_t pageCount);
    void LockPage(void* address);
    void LockPages(void* address, uint64_t pageCount);
    void* RequestPage();
    void *RequestPages(uint32_t count);
    uint64_t GetFreeRAM();
    uint64_t GetUsedRAM();
    uint64_t GetReservedRAM();

private:
    void InitBitmap(size_t bitmapSize, void* bufferAddress, volatile limine_hhdm_request *hhdm);
    void ReservePage(void* address);
    void ReservePages(void* address, uint64_t pageCount);
    void UnreservePage(void* address);
    void UnreservePages(void* address, uint64_t pageCount);
};

extern PageFrameAllocator globalAllocator;
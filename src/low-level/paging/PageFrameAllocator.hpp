#pragma once
#include <stivale2.h>
#include <stdint.h>
#include "Bitmap.hpp"

class PageFrameAllocator
{
public:
    Bitmap PageBitmap;

    void ReadMemoryMap(stivale2_struct_tag_memmap *memmap);
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
    void InitBitmap(size_t bitmapSize, void* bufferAddress);
    void ReservePage(void* address);
    void ReservePages(void* address, uint64_t pageCount);
    void UnreservePage(void* address);
    void UnreservePages(void* address, uint64_t pageCount);
};

extern PageFrameAllocator globalAllocator;
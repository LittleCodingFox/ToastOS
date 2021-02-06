#pragma once
#include "efimemory/EFIMemory.hpp"
#include <stdint.h>
#include "Bitmap.hpp"

class PageFrameAllocator
{
public:
    Bitmap PageBitmap;

    void readEFIMemoryMap(EFI_MEMORY_DESCRIPTOR* mMap, size_t mMapSize, size_t mMapDescSize);
    void freePage(void* address);
    void freePages(void* address, uint64_t pageCount);
    void lockPage(void* address);
    void lockPages(void* address, uint64_t pageCount);
    void* requestPage();
    void *requestPages(uint32_t count);
    uint64_t getFreeRAM();
    uint64_t getUsedRAM();
    uint64_t getReservedRAM();

private:
    void initBitmap(size_t bitmapSize, void* bufferAddress);
};

extern PageFrameAllocator globalAllocator;
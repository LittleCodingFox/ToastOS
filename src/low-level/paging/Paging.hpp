#pragma once
#include <stdint.h>

#define HIGHER_HALF_MEMORY_OFFSET           0xFFFF800000000000
#define HIGHER_HALF_KERNEL_MEMORY_OFFSET    0xFFFFFFFF80000000

#define PAGE_FLAG_MASK                      (0xFFF | (1ull << 63))
#define PAGE_ADDRESS_MASK                   (~(PAGE_FLAG_MASK))

inline uint64_t TranslateToHighHalfMemoryAddress(uint64_t physicalAddress)
{
    return physicalAddress + HIGHER_HALF_MEMORY_OFFSET;
}

inline uint64_t TranslateToPhysicalMemoryAddress(uint64_t virtualAddress)
{
    return virtualAddress - HIGHER_HALF_MEMORY_OFFSET;
}

inline uint64_t TranslateToKernelPhysicalMemoryAddress(uint64_t virtualAddress)
{
    return virtualAddress - HIGHER_HALF_KERNEL_MEMORY_OFFSET;
}

inline uint64_t TranslateToKernelMemoryAddress(uint64_t virtualAddress)
{
    return virtualAddress + HIGHER_HALF_KERNEL_MEMORY_OFFSET;
}

inline bool IsHigherHalf(uint64_t physicalAddress)
{
    return physicalAddress >= HIGHER_HALF_MEMORY_OFFSET;
}

inline bool IsKernelHigherHalf(uint64_t physicalAddress)
{
    return physicalAddress >= HIGHER_HALF_KERNEL_MEMORY_OFFSET;
}

enum PagingFlag
{
    PAGING_FLAG_PRESENT = (1ull << 0),
    PAGING_FLAG_WRITABLE = (1ull << 1),
    PAGING_FLAG_USER_ACCESSIBLE = (1ull << 2),
    PAGING_FLAG_LARGER_PAGES = (1ull << 7),
    PAGING_FLAG_NO_EXECUTE = (1ull << 63),
};

struct PageTableOffset
{
    uint64_t p4Offset;
    uint64_t pdpOffset;
    uint64_t pdOffset;
    uint64_t ptOffset;
};

struct PageTable
{
    uint64_t entries[512];
}__attribute__((aligned(0x1000)));

PageTableOffset VirtualAddressToOffsets(void *address);
void *OffsetToVirtualAddress(PageTableOffset offset);

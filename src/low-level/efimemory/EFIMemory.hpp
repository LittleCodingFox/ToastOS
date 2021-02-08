#pragma once
#include <stdint.h>

struct EFI_MEMORY_DESCRIPTOR {

    uint32_t type;
    uint32_t padding;
    uint64_t physAddr;
    uint64_t virtAddr; 
    uint64_t numPages;
    uint64_t attribs;
};

extern const char* EFI_MEMORY_TYPE_STRINGS[];

uint64_t getMemorySize(EFI_MEMORY_DESCRIPTOR* mMap, uint64_t mMapEntries, uint64_t mMapDescSize);

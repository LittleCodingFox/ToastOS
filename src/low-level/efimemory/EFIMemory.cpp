#include "EFIMemory.hpp"

const char* EFI_MEMORY_TYPE_STRINGS[] {

    "EfiReservedMemoryType",
    "EfiLoaderCode",
    "EfiLoaderData",
    "EfiBootServicesCode",
    "EfiBootServicesData",
    "EfiRuntimeServicesCode",
    "EfiRuntimeServicesData",
    "EfiConventionalMemory",
    "EfiUnusableMemory",
    "EfiACPIReclaimMemory",
    "EfiACPIMemoryNVS",
    "EfiMemoryMappedIO",
    "EfiMemoryMappedIOPortSpace",
    "EfiPalCode",
};

uint64_t GetMemorySize(volatile EFI_MEMORY_DESCRIPTOR *mMap, uint64_t mMapEntries, uint64_t mMapDescSize)
{
    static uint64_t memorySizeBytes = 0;

    if (memorySizeBytes > 0) return memorySizeBytes;

    for (int i = 0; i < mMapEntries; i++)
    {
        volatile EFI_MEMORY_DESCRIPTOR* desc = (volatile EFI_MEMORY_DESCRIPTOR *)((uint64_t)mMap + (i * mMapDescSize));

        memorySizeBytes += desc->numPages * 4096;
    }

    return memorySizeBytes;
}

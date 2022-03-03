#include "GenericIODevice.hpp"
#include "debug.hpp"
#include <string.h>

bool GenericIODevice::ReadUnaligned(void *data, uint64_t sector, uint64_t count)
{
    uint64_t maxBlocks = ((sector % 512) + count) / 512 + 1;
    uint64_t currentSector = ((sector / 512)) * 512;
    uint8_t *buffer = new uint8_t[maxBlocks * 512];

    if(!Read(buffer, currentSector / 512, maxBlocks))
    {
        delete [] buffer;

        return false;
    }

    memcpy(data, buffer + sector % 512, count);

    delete [] buffer;

    return true;
}

bool GenericIODevice::WriteUnaligned(const void *data, uint64_t sector, uint64_t count)
{
    uint64_t maxBlocks = ((sector % 512) + count) / 512 + 1;
    uint64_t currentSector = ((sector / 512)) * 512;
    uint8_t *buffer = new uint8_t[maxBlocks * 512];

    if(!ReadUnaligned(buffer, maxBlocks * 512, currentSector))
    {
        return false;
    }

    memcpy(buffer + sector % 512, data, count);

    if(!Write(buffer, currentSector / 512, maxBlocks))
    {
        delete [] buffer;

        return false;
    }

    delete [] buffer;

    return true;
}

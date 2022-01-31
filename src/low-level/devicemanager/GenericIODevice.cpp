#include "GenericIODevice.hpp"
#include "debug.hpp"
#include <string.h>

bool GenericIODevice::ReadUnaligned(void *data, uint64_t sector, uint64_t count)
{
    uint64_t maxBlocks = ((sector % 512) + count) / 512 + 1;
    uint64_t currentSector = ((sector / 512)) * 512;
    uint8_t *buffer = (uint8_t *)malloc(maxBlocks * 512);

    for(uint64_t i = 0; i < maxBlocks; i++)
    {
        bool result = Read(buffer + (i * 512), i + (currentSector / 512), 1);

        if(!result)
        {
            free(buffer);

            return false;
        }
    }

    memcpy(data, buffer + sector % 512, count);
    free(buffer);

    return true;
}

bool GenericIODevice::WriteUnaligned(const void *data, uint64_t sector, uint64_t count)
{
    uint64_t maxBlocks = ((sector % 512) + count) / 512 + 1;
    uint64_t currentSector = ((sector / 512)) * 512;
    uint8_t *buffer = (uint8_t *)malloc(maxBlocks * 512);
    ReadUnaligned(buffer, maxBlocks * 512, currentSector);

    memcpy(buffer + sector % 512, data, count);

    for(uint64_t i = 0; i < maxBlocks; i++)
    {
        bool result = Write(buffer + (i * 512), i + (currentSector / 512), 1);

        if(!result)
        {
            free(buffer);

            return false;
        }
    }

    free(buffer);

    return true;
}

#include "GenericIODevice.hpp"

namespace Devices
{
    bool GenericIODevice::ReadUnaligned(void *data, uint64_t sector, uint64_t count)
    {
        uint64_t maxBlocks = ((sector % 512) + count) / 512 + 1;
        uint8_t *buffer = (uint8_t *)malloc(maxBlocks * 512);

        for(uint32_t i = 0; i < maxBlocks; i++)
        {
            bool result = Read(buffer + (i * 512), 1, i + sector / 512);

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

    bool GenericIODevice::WriteUnaligned(void *data, uint64_t sector, uint64_t count)
    {
        uint64_t maxBlocks = ((sector % 512) + count) / 512 + 1;
        uint64_t currentSector = ((sector / 512)) * 512;
        uint8_t *buffer = (uint8_t *)malloc(maxBlocks * 512);
        ReadUnaligned(buffer, maxBlocks * 512, currentSector);

        memcpy(buffer + sector % 12, data, count);

        for(uint32_t i = 0; i < maxBlocks; i++)
        {
            bool result = Write(buffer + (i * 512), 1, i + (currentSector / 512));

            if(!result)
            {
                free(buffer);

                return false;
            }
        }

        free(buffer);

        return true;
    }
}

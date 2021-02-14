#include "ACPI.hpp"
#include <string.h>

namespace ACPI
{
    void *findTable(SDTHeader *header, char *signature)
    {
        uint32_t entries = (header->length - sizeof(SDTHeader)) / 8;

        for(uint32_t i = 0, index = 0; i < entries; i++, index += 8)
        {
            SDTHeader *outHeader = (SDTHeader *)*(uint64_t *)((uint64_t)header) + sizeof(SDTHeader) + index;

            if(memcmp(outHeader->signature, signature, sizeof(char[4])) == 0)
            {
                return outHeader;
            }
        }

        return NULL;
    }
}

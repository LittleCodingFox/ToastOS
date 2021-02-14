#include "ACPI.hpp"
#include "debug.hpp"
#include <string.h>

namespace ACPI
{
    void *findTable(SDTHeader *header, char *signature)
    {
        uint32_t entries = (header->length - sizeof(SDTHeader)) / sizeof(uint64_t);
        uint64_t *entryPtr = (uint64_t *)(header + 1);

        for(uint32_t i = 0; i < entries; i++, entryPtr++)
        {
            SDTHeader *outHeader = (SDTHeader *)(*entryPtr);

            if(memcmp(outHeader->signature, signature, sizeof(char[4])) == 0)
            {
                return outHeader;
            }
        }

        return NULL;
    }
}

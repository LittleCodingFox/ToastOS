#include "ACPI.hpp"
#include "debug.hpp"
#include "paging/Paging.hpp"
#include <string.h>

namespace ACPI
{
    struct [[gnu::packed]] XSDT
    {
        SDTHeader header;
        uint64_t entries[];
    };

    void DumpTables(volatile SDTHeader *header)
    {
        uint32_t entries = (header->length - sizeof(SDTHeader)) / sizeof(uint64_t);
        volatile XSDT *xsdt = reinterpret_cast<volatile XSDT *>(header);

        for(uint32_t i = 0; i < entries; i++)
        {
            SDTHeader *outHeader = (SDTHeader *)TranslateToHighHalfMemoryAddress(xsdt->entries[i]);

            printf("[ACPI] Table entry %u: %.*s\n", i, 4, outHeader->signature);
        }
    }

    void *FindTable(volatile SDTHeader *header, const char *signature)
    {
        uint32_t entries = (header->length - sizeof(SDTHeader)) / sizeof(uint64_t);
        volatile XSDT *xsdt = reinterpret_cast<volatile XSDT *>(header);

        for(uint32_t i = 0; i < entries; i++)
        {
            SDTHeader *outHeader = (SDTHeader *)TranslateToHighHalfMemoryAddress(xsdt->entries[i]);

            if(memcmp(outHeader->signature, signature, 4) == 0)
            {
                return outHeader;
            }
        }

        return NULL;
    }
}

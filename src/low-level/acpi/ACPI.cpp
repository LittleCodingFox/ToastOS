#include "ACPI.hpp"
#include "debug.hpp"
#include "paging/PageTableManager.hpp"
#include <string.h>
#include <lai/core.h>
#include <lai/helpers/sci.h>
#include <lai/helpers/pm.h>

namespace ACPI
{
    struct PACKED XSDT
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
            volatile SDTHeader *outHeader = (volatile SDTHeader *)TranslateToHighHalfMemoryAddress(xsdt->entries[i]);
            (void)outHeader;

            DEBUG_OUT("[ACPI] Table entry %u: %.*s", i, 4, outHeader->signature);
        }
    }

    void *FindTable(volatile SDTHeader *header, const char *signature)
    {
        return FindTable(header, signature, -1);
    }

    void *FindTable(volatile SDTHeader *header, const char *signature, int index)
    {
        if(strcmp(signature, "DSDT") == 0)
        {
            acpi_fadt_t *fadt = (acpi_fadt_t *)FindTable(header, "FACP");

            if(fadt == nullptr)
            {
                return nullptr;
            }

            uint64_t dsdt = fadt->x_dsdt;

            return (void *)TranslateToHighHalfMemoryAddress(dsdt);
        }

        uint32_t entries = (header->length - sizeof(SDTHeader)) / sizeof(uint64_t);
        volatile XSDT *xsdt = reinterpret_cast<volatile XSDT *>(header);

        for(uint32_t i = 0; i < entries; i++)
        {
            volatile SDTHeader *outHeader = (volatile SDTHeader *)TranslateToHighHalfMemoryAddress(xsdt->entries[i]);

            if(memcmp((const void *)outHeader->signature, signature, 4) == 0 && (index == -1 || index == i))
            {
                return (void *)outHeader;
            }
        }

        return NULL;
    }
}

#include "PageTableManager.hpp"
#include "PageMapIndexer.hpp"
#include <stdint.h>
#include <string.h>
#include "PageFrameAllocator.hpp"
#include "debug.hpp"
#include "registers/Registers.hpp"

PageTableManager *globalPageTableManager = NULL;

static inline PageTable *GetOrAllocEntry(PageTable *table, uint64_t offset, uint64_t flags, PageTableManager *self)
{
    uint64_t address = table->entries[offset];

    if(!(address & PAGING_FLAG_PRESENT))
    {
        address = table->entries[offset] = (uint64_t)globalAllocator.RequestPage();

        if(!address)
        {
            return NULL;
        }

        table->entries[offset] |= flags | PAGING_FLAG_PRESENT;

        //Ensure we have the address mapped when accessing it. If we don't, we need to make it usable in the current P4
        if((uint64_t)self->p4 != Registers::ReadCR3() && globalPageTableManager != NULL)
        {
            PageTableManager temp;
            temp.p4 = (PageTable *)Registers::ReadCR3();

            temp.MapMemory((void *)TranslateToHighHalfMemoryAddress(address), (void *)address, flags);
        }

        memset((void *)TranslateToHighHalfMemoryAddress(address), 0, 0x1000);
    }

    return (PageTable *)TranslateToHighHalfMemoryAddress(address & PAGE_ADDRESS_MASK);
}

static inline PageTable *GetOrNullifyEntry(PageTable *table, uint64_t offset)
{
    uint64_t address = table->entries[offset];

    if(!(address & PAGING_FLAG_PRESENT))
    {
        return NULL;
    }

    return (PageTable *)TranslateToHighHalfMemoryAddress(address & PAGE_ADDRESS_MASK);
}

PageTableManager::PageTableManager() : p4(NULL)
{
}

void PageTableManager::IdentityMap(void *physicalMemory, uint64_t flags)
{
    MapMemory(physicalMemory, physicalMemory, flags);
}

void PageTableManager::MapMemory(void *virtualMemory, void *physicalMemory, uint64_t flags)
{
    uint64_t higherPermissions = PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE;

    PageTableOffset offset = VirtualAddressToOffsets(virtualMemory);

    PageTable *p4Virtual = (PageTable *)TranslateToHighHalfMemoryAddress((uint64_t)p4);

    PageTable *pdp = GetOrAllocEntry(p4Virtual, offset.p4Offset, higherPermissions, this);
    PageTable *pd = GetOrAllocEntry(pdp, offset.pdpOffset, higherPermissions, this);
    PageTable *pt = GetOrAllocEntry(pd, offset.pdOffset, higherPermissions, this);

    pt->entries[offset.ptOffset] = (uint64_t)physicalMemory | flags | PAGING_FLAG_PRESENT;
}

void PageTableManager::UnmapMemory(void *virtualMemory)
{
    PageTableOffset offset = VirtualAddressToOffsets(virtualMemory);

    PageTable *p4Virtual = (PageTable *)TranslateToHighHalfMemoryAddress((uint64_t)p4);
    PageTable *pdp = GetOrNullifyEntry(p4Virtual, offset.p4Offset);

    if(pdp == NULL)
    {
        return;
    }

    PageTable *pd = GetOrNullifyEntry(pdp, offset.pdpOffset);

    if(pd == NULL)
    {
        return;
    }

    PageTable *pt = GetOrNullifyEntry(pd, offset.pdOffset);

    if(pt == NULL)
    {
        return;
    }

    pt->entries[offset.ptOffset] = 0;
}

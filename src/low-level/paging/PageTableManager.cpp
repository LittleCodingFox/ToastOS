#include "PageTableManager.hpp"
#include "PageMapIndexer.hpp"
#include <stdint.h>
#include <string.h>
#include "PageFrameAllocator.hpp"
#include "debug.hpp"

PageTableManager *globalPageTableManager = NULL;

PageTableManager::PageTableManager(PageTable *PML4Address)
{
    this->PML4 = PML4Address;
}

void PageTableManager::IdentityMap(void *physicalMemory)
{
    MapMemory(physicalMemory, physicalMemory);
}

void PageTableManager::MapMemory(void *virtualMemory, void *physicalMemory)
{
    PageMapIndexer indexer = PageMapIndexer((uint64_t)virtualMemory);
    PageDirectoryEntry PDE;

    PDE = PML4->entries[indexer.PDP_i];
    PageTable* PDP;

    if (!PDE.present)
    {
        PDP = (PageTable*)globalAllocator.RequestPage();
        memset(PDP, 0, 0x1000);
        PDE.SetAddress((uint64_t)PDP >> 12);
        PDE.present = true;
        PDE.writable = true;
        PML4->entries[indexer.PDP_i] = PDE;
    }
    else
    {
        PDP = (PageTable*)((uint64_t)PDE.GetAddress() << 12);
    }
    
    PDE = PDP->entries[indexer.PD_i];
    PageTable* PD;

    if (!PDE.present)
    {
        PD = (PageTable*)globalAllocator.RequestPage();
        memset(PD, 0, 0x1000);
        PDE.SetAddress((uint64_t)PD >> 12);
        PDE.present = true;
        PDE.writable = true;
        PDP->entries[indexer.PD_i] = PDE;
    }
    else
    {
        PD = (PageTable*)((uint64_t)PDE.GetAddress() << 12);
    }

    PDE = PD->entries[indexer.PT_i];
    PageTable* PT;

    if (!PDE.present)
    {
        PT = (PageTable*)globalAllocator.RequestPage();
        memset(PT, 0, 0x1000);
        PDE.SetAddress((uint64_t)PT >> 12);
        PDE.present = true;
        PDE.writable = true;
        PD->entries[indexer.PT_i] = PDE;
    }
    else
    {
        PT = (PageTable*)((uint64_t)PDE.GetAddress() << 12);
    }

    PDE = PT->entries[indexer.P_i];
    PDE.SetAddress((uint64_t)physicalMemory >> 12);
    PDE.present = true;
    PDE.writable = true;
    PT->entries[indexer.P_i] = PDE;
}

void PageTableManager::UnmapMemory(void *virtualMemory)
{
    PageMapIndexer indexer = PageMapIndexer((uint64_t)virtualMemory);
    PageDirectoryEntry PDE;

    PDE = PML4->entries[indexer.PDP_i];
    PageTable* PDP;

    if (!PDE.present)
    {
        PDP = (PageTable*)globalAllocator.RequestPage();
        memset(PDP, 0, 0x1000);
        PDE.SetAddress((uint64_t)PDP >> 12);
        PDE.present = true;
        PDE.writable = true;
        PML4->entries[indexer.PDP_i] = PDE;
    }
    else
    {
        PDP = (PageTable*)((uint64_t)PDE.GetAddress() << 12);
    }
    
    PDE = PDP->entries[indexer.PD_i];
    PageTable* PD;

    if (!PDE.present)
    {
        PD = (PageTable*)globalAllocator.RequestPage();
        memset(PD, 0, 0x1000);
        PDE.SetAddress((uint64_t)PD >> 12);
        PDE.present = true;
        PDE.writable = true;
        PDP->entries[indexer.PD_i] = PDE;
    }
    else
    {
        PD = (PageTable*)((uint64_t)PDE.GetAddress() << 12);
    }

    PDE = PD->entries[indexer.PT_i];
    PageTable* PT;

    if (!PDE.present)
    {
        PT = (PageTable*)globalAllocator.RequestPage();
        memset(PT, 0, 0x1000);
        PDE.SetAddress((uint64_t)PT >> 12);
        PDE.present = true;
        PDE.writable = true;
        PD->entries[indexer.PT_i] = PDE;
    }
    else
    {
        PT = (PageTable*)((uint64_t)PDE.GetAddress() << 12);
    }

    PDE = PT->entries[indexer.P_i];
    PDE.SetAddress((uint64_t)virtualMemory >> 12);
    PDE.present = false;
    PDE.writable = true;
    PT->entries[indexer.P_i] = PDE;
}

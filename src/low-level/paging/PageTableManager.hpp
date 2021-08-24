#pragma once
#include "Paging.hpp"

class PageTableManager
{
public:
    PageTableManager(PageTable* PML4Address);
    PageTable* PML4;
    void MapMemory(void* virtualMemory, void* physicalMemory, PagingFlag::PagingFlag flags);
    void UnmapMemory(void* virtualMemory);
    void IdentityMap(void *physicalMemory);
};

extern PageTableManager *globalPageTableManager;

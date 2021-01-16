#pragma once
#include "Paging.hpp"

class PageTableManager
{
public:
    PageTableManager(PageTable* PML4Address);
    PageTable* PML4;
    void MapMemory(void* virtualMemory, void* physicalMemory);
    void UnmapMemory(void* virtualMemory);
};

extern PageTableManager *GlobalPageTableManager;

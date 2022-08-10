#pragma once
#include "osdev-paging-x64/paging.h"

class PageTableManager
{
public:
    PageTableManager();
    PageTable* p4;
    void MapMemory(void* virtualMemory, void* physicalMemory, uint64_t flags);
    void UnmapMemory(void* virtualMemory);
    void IdentityMap(void *physicalMemory, uint64_t flags);
    void *PhysicalMemory(void *virtualMemory);
    void Duplicate(PageTable *newTable);
};

extern PageTableManager *globalPageTableManager;

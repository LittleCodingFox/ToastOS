#include "PageTableManager.hpp"
#include "PageMapIndexer.hpp"
#include <stdint.h>
#include <string.h>
#include "PageFrameAllocator.hpp"
#include "debug.hpp"
#include "Panic.hpp"
#include "registers/Registers.hpp"
#include "stacktrace/stacktrace.hpp"

PageTableManager *globalPageTableManager = NULL;

PageTableManager::PageTableManager() : p4(NULL)
{
}

void PageTableManager::IdentityMap(void *physicalMemory, uint64_t flags)
{
    PagingMapMemory(p4, physicalMemory, physicalMemory, flags);
}

void PageTableManager::MapMemory(void *virtualMemory, void *physicalMemory, uint64_t flags)
{
    PagingMapMemory(p4, virtualMemory, physicalMemory, flags);
}

void *PageTableManager::PhysicalMemory(void *virtualMemory)
{
    return PagingPhysicalMemory(p4, virtualMemory);
}

void PageTableManager::UnmapMemory(void *virtualMemory)
{
    PagingUnmapMemory(p4, virtualMemory);
}

void PageTableManager::Duplicate(PageTable *newTable)
{
    PagingDuplicate(p4, newTable);
}

uint64_t PagingGetFreeFrame()
{
    return (uint64_t)globalAllocator.RequestPage();
}

#include "VMM.hpp"
#include "Panic.hpp"
#include "paging/PageFrameAllocator.hpp"

BackingPage::BackingPage() : ptr(0), allocated(false), exists(false) {}

MemoryMapping::MemoryMapping(uint64_t base, uint64_t size) : base(base), size(size), flags(0),
    mapped(false) {

    pages.resize(size);
}

MemoryMapping::~MemoryMapping()
{
    for(auto &page : pages)
    {
        if(page.exists && page.allocated && page.ptr)
        {
            globalAllocator.FreePage((void *)page.ptr);
        }
    }
}

void MemoryMapping::Allocate(uint64_t flags)
{
    this->flags = flags;

    for(auto &page : pages)
    {
        if(page.exists || page.allocated)
        {
            Panic("Assertion failed: Double allocating");
        }

        page.allocated = true;
    }
}

void MemoryMapping::MapTo(uint64_t address, uint64_t flags)
{
    this->flags = flags;

    for(auto &page : pages)
    {
        if(page.exists || page.allocated)
        {
            Panic("Assertion failed: Double mapping");
        }

        page.exists = true;
        page.allocated = false;
        page.ptr = address;
        address += 0x1000;
    }
}

void MemoryMapping::Deallocate()
{
    if(mapped)
    {
        Panic("Deallocating a mapped memory map");
    }

    for(auto &page : pages)    
    {
        bool alloc = false;

        if(page.allocated)
        {
            page.allocated = false;
            alloc = true;
        }

        if(page.exists)
        {
            if(alloc)
            {
                globalAllocator.FreePage((void *)page.ptr);
            }

            page.exists = false;
        }

        page.ptr = 0;
    }
}

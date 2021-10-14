#pragma once

#include <stddef.h>
#include <stdint.h>
#include <new>
#include <frg/vector.hpp>
#include <frg_allocator.hpp>

struct BackingPage
{
    uint64_t ptr;
    bool exists;
    bool allocated;

    BackingPage();
};

struct MemoryMapping
{
    uint64_t base;
    uint64_t size;
    uint64_t flags;
    bool mapped;

    frg::vector<BackingPage, frg_allocator> pages;

    MemoryMapping(uint64_t base, uint64_t size);
    ~MemoryMapping();

    void Allocate(uint64_t flags);
    void MapTo(uint64_t address, uint64_t flags);
    void Deallocate();
};

struct MemoryHole
{
    uint64_t base;
    uint64_t size;
};

struct AddressSpace
{
    AddressSpace();
    ~AddressSpace();

    MemoryHole *FindFreeHole(uint64_t size);
    MemoryMapping *BindHole(MemoryHole *hole, uint64_t size);
    MemoryMapping *BindExact(uint64_t base, uint64_t size);

    void UnbindRegion(MemoryMapping *region);
    void MapRegion(MemoryMapping *region);
    void UnmapRegion(MemoryMapping *region);

    uint64_t Allocate(uint64_t size, uint64_t flags);
    uint64_t Map(uint64_t address, uint64_t size, uint64_t flags);

    uint64_t AllocateAt(uint64_t base, uint64_t size, uint64_t flags);
    uint64_t MapAt(uint64_t base, uint64_t address, uint64_t size, uint64_t flags);

    void Destroy(MemoryMapping *region);
    void Destroy(uint64_t address);

    MemoryMapping *RegionForAddress(uint64_t address);
private:

    MemoryHole *FindPreviousHole(MemoryHole *in);
    MemoryMapping *FindPreviousMapping(MemoryMapping *in);

    MemoryHole *FindNextHole(MemoryHole *in);
    MemoryMapping *FindNextMapping(MemoryMapping *in);

    void Put(MemoryMapping *mapping);
    void Put(MemoryHole *hole);

    void MergeHoles();
};

#include "Paging.hpp"

void PageDirectoryEntry::setFlag(PT_Flag flag, bool enabled)
{
    uint64_t bitSelector = (uint64_t)1 << flag;
    Value &= ~bitSelector;
    
    if (enabled)
    {
        Value |= bitSelector;
    }
}

bool PageDirectoryEntry::getFlag(PT_Flag flag)
{
    uint64_t bitSelector = (uint64_t)1 << flag;

    return (Value & bitSelector) > 0 ? true : false;
}

uint64_t PageDirectoryEntry::getAddress()
{
    return (Value & 0x000ffffffffff000) >> 12;
}

void PageDirectoryEntry::setAddress(uint64_t address)
{
    address &= 0x000000ffffffffff;
    Value &= 0xfff0000000000fff;
    Value |= (address << 12);
}

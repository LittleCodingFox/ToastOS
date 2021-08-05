#include "Paging.hpp"

uint64_t PageDirectoryEntry::GetAddress()
{
    return (packed & 0x000ffffffffff000) >> 12;
}

void PageDirectoryEntry::SetAddress(uint64_t address)
{
    address &= 0x000000ffffffffff;
    packed &= 0xfff0000000000fff;
    packed |= (address << 12);
}

#include "Paging.hpp"

uint64_t PageDirectoryEntry::getAddress()
{
    return (packed & 0x000ffffffffff000) >> 12;
}

void PageDirectoryEntry::setAddress(uint64_t address)
{
    address &= 0x000000ffffffffff;
    packed &= 0xfff0000000000fff;
    packed |= (address << 12);
}

#include "MMIO.hpp"

uint8_t MMIORead8 (uint64_t p_address)
{
    return *((volatile uint8_t*)(p_address));
}

uint16_t MMIORead16 (uint64_t p_address)
{
    return *((volatile uint16_t*)(p_address));
}

uint32_t MMIORead32 (uint64_t p_address)
{
    return *((volatile uint32_t*)(p_address));
}

uint64_t MMIORead64 (uint64_t p_address)
{
    return *((volatile uint64_t*)(p_address));    
}

void MMIOWrite8 (uint64_t p_address,uint8_t p_value)
{
    (*((volatile uint8_t*)(p_address)))=(p_value);
}

void MMIOWrite16 (uint64_t p_address,uint16_t p_value)
{
    (*((volatile uint16_t*)(p_address)))=(p_value);    
}

void MMIOWrite32 (uint64_t p_address,uint32_t p_value)
{
    (*((volatile uint32_t*)(p_address)))=(p_value);
}

void MMIOWrite64 (uint64_t p_address,uint64_t p_value)
{
    (*((volatile uint64_t*)(p_address)))=(p_value);    
}

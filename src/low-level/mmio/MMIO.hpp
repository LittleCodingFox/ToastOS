#pragma once

#include <stddef.h>
#include <stdint.h>

uint8_t MMIORead8(uint64_t address);
uint16_t MMIORead16(uint64_t address);
uint32_t MMIORead32(uint64_t address);
uint64_t MMIORead64(uint64_t address);

void MMIOWrite8(uint64_t address, uint8_t value);
void MMIOWrite16(uint64_t address, uint16_t value);
void MMIOWrite32(uint64_t address, uint32_t value);
void MMIOWrite64(uint64_t address, uint64_t value);

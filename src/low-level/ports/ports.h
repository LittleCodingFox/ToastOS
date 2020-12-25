#ifndef PORTS_H
#define PORTS_H
#include <stdint.h>

uint8_t inport8(uint16_t port);
void outport8(uint16_t port, uint8_t value);
uint16_t inport16(uint16_t port);
void outport16(uint16_t port, uint16_t value);
uint32_t inport32(uint16_t port);
void outport32(uint16_t port, uint32_t value);

#endif

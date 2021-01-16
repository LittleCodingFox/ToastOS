#pragma once
#include <stdint.h>

void outport8(uint16_t port, uint8_t value);
uint8_t inport8(uint16_t port);
void io_wait();

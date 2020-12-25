#include "ports.h"

uint8_t inport8(uint16_t port) {

    uint8_t outValue = 0;

    __asm__("inb %1, %0" : "=a"(outValue) : "Nd"(port));

    return outValue;
}

void outport8(uint16_t port, uint8_t value) {

    __asm__("outb %0, %1" : : "a"(value), "Nd"(port));
}

uint16_t inport16(uint16_t port) {

    uint16_t outValue = 0;
    
    __asm__("inw %1, %0" : "=a"(outValue) : "Nd"(port));

    return outValue;
}

void outport16(uint16_t port, uint16_t value) {

   __asm__("outw %0, %1" : : "a"(value), "Nd"(port));
}

uint32_t inport32(uint16_t port) {

    uint32_t outValue = 0;
    
    __asm__("inl %1, %0" : "=a"(outValue) : "Nd"(port));

    return outValue;
}

void outport32(uint16_t port, uint32_t value) {

   __asm__("outl %0, %1" : : "a"(value), "Nd"(port));
}

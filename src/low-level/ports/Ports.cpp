#include "Ports.hpp"

void outport8(uint16_t port, uint8_t value)
{
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

uint8_t inport8(uint16_t port)
{
    uint8_t outValue;

    asm volatile ("inb %1, %0"
        : "=a"(outValue)
        : "Nd"(port));

    return outValue;
}

uint16_t inport16(uint16_t port)
{
    uint16_t outValue = 0;

    asm volatile("inw %1, %0"
        : "=a"(outValue)
        : "Nd"(port));

    return outValue;
}

void outport16(uint16_t port, uint16_t value)
{
  asm volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

uint32_t inport32(uint16_t port)
{
  uint32_t outValue = 0;

  asm volatile ("inl %1, %0" : "=a"(outValue) : "Nd"(port));

  return outValue;
}

void outport32(uint16_t port, uint32_t value)
{
  asm volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

void io_wait()
{
    asm volatile ("outb %%al, $0x80" : : "a"(0));
}

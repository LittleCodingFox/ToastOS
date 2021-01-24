#include "IDT.hpp"
#include <string.h>

#define KERNEL_BASE_SELECTOR 0x08

struct __attribute__((packed)) IDTR
{
    uint16_t limit;
    uint64_t base;
};

IDT idt;

void IDT::init()
{
    memset(idt, 0, sizeof(idt));
}

void IDT::load()
{
    IDTR idtRegister = {
        .base = (uint64_t)idt,
        .limit = (uint16_t)(sizeof(idt) - 1)
    };

    asm ("lidt %0" : : "m"(idtRegister));
}

void IDT::registerGate(uint16_t n, uint64_t handler, uint8_t type, uint8_t dpl)
{
    idt[n].ptrLow = (uint16_t)handler;
    idt[n].ptrMid = (uint16_t)(handler >> 16);
    idt[n].ptrHigh = (uint32_t)(handler >> 32);
    idt[n].selector = KERNEL_BASE_SELECTOR;
    idt[n].ist = 0;
    idt[n].type = type;
    idt[n].s = 0;
    idt[n].dpl = dpl;
    idt[n].present = 1;
}

void IDT::registerInterrupt(uint16_t n, uint64_t handler)
{
    registerGate(n, handler, IDT_INTERRUPT_GATE, 0);
}

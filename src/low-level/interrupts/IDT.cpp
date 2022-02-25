#include "IDT.hpp"
#include "gdt/gdt.hpp"
#include <string.h>

struct PACKED IDTR
{
    uint16_t limit;
    uint64_t base;
};

IDT idt;

void IDT::Init()
{
    memset(idt, 0, sizeof(idt));
}

void IDT::Load()
{
    IDTR idtRegister = {
        .limit = (uint16_t)(sizeof(idt) - 1),
        .base = (uint64_t)idt,
    };

    asm ("lidt %0" : : "m"(idtRegister));
}

void IDT::RegisterGate(uint16_t n, uint64_t handler, uint8_t type, uint8_t dpl, uint8_t ist)
{
    idt[n].ptrLow = (uint16_t)handler;
    idt[n].ptrMid = (uint16_t)(handler >> 16);
    idt[n].ptrHigh = (uint32_t)(handler >> 32);
    idt[n].selector = GDTKernelBaseSelector;
    idt[n].ist = ist;
    idt[n].type = type;
    idt[n].s = 0;
    idt[n].dpl = dpl;
    idt[n].present = 1;
}

void IDT::RegisterInterrupt(uint16_t n, uint64_t handler, uint8_t dpl, uint8_t ist)
{
    RegisterGate(n, handler, IDT_INTERRUPT_GATE, dpl, ist);
}

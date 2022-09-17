#include "IDT.hpp"
#include "gdt/gdt.hpp"
#include "Panic.hpp"
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

static AtomicLock lock;

uint8_t IDT::AllocateVector()
{
    lock.Lock();

    if(freeVector == 0xF0)
    {
        Panic("IDT Exhausted");
    }

    uint8_t outValue = freeVector++;

    lock.Unlock();

    return outValue;
}

void IDT::SetIST(uint8_t vector, uint8_t ist)
{
    idt[vector].ist = ist;
}

void IDT::SetFlags(uint8_t vector, uint8_t flags)
{
    idt[vector].flags = flags;
}

void IDT::RegisterGate(uint8_t vector, uint64_t handler, uint8_t type, uint8_t dpl, uint8_t ist)
{
    idt[vector].ptrLow = (uint16_t)handler;
    idt[vector].ptrMid = (uint16_t)(handler >> 16);
    idt[vector].ptrHigh = (uint32_t)(handler >> 32);
    idt[vector].selector = GDTKernelBaseSelector;
    idt[vector].ist = ist;
    idt[vector].type = type;
    idt[vector].s = 0;
    idt[vector].dpl = dpl;
    idt[vector].present = 1;
}

void IDT::RegisterInterrupt(uint8_t vector, uint64_t handler, uint8_t dpl, uint8_t ist)
{
    RegisterGate(vector, handler, IDT_INTERRUPT_GATE, dpl, ist);
}

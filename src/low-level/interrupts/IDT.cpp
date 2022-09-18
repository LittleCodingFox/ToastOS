#include "IDT.hpp"
#include "Interrupts.hpp"
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
    memset(entries, 0, sizeof(entries));

    freeVector = 32;
}

void IDT::Load()
{
    IDTR idtRegister = {
        .limit = (uint16_t)(sizeof(entries) - 1),
        .base = (uint64_t)entries,
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

    if(outValue == IRQ1 || outValue == IRQ12)
    {
        outValue = freeVector++;
    }

    lock.Unlock();

    return outValue;
}

void IDT::SetIST(uint8_t vector, uint8_t ist)
{
    entries[vector].ist = ist;
}

void IDT::SetFlags(uint8_t vector, uint8_t flags)
{
    entries[vector].flags = flags;
}

void IDT::RegisterGate(uint8_t vector, uint64_t handler, uint8_t type, uint8_t dpl, uint8_t ist)
{
    entries[vector].ptrLow = (uint16_t)handler;
    entries[vector].ptrMid = (uint16_t)(handler >> 16);
    entries[vector].ptrHigh = (uint32_t)(handler >> 32);
    entries[vector].selector = GDTKernelBaseSelector;
    entries[vector].ist = ist;
    entries[vector].type = type;
    entries[vector].s = 0;
    entries[vector].dpl = dpl;
    entries[vector].present = 1;
}

void IDT::RegisterInterrupt(uint8_t vector, uint64_t handler, uint8_t dpl, uint8_t ist)
{
    RegisterGate(vector, handler, IDT_INTERRUPT_GATE, dpl, ist);
}

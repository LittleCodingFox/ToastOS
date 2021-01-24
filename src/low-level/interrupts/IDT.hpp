#pragma once
#include <stdint.h>

#define IDT_INTERRUPT_GATE  0xE
#define IDT_TRAP_GATE       0xF
#define IDT_ENTRIES         256

struct __attribute__((packed)) IDTDescEntry
{
    uint16_t ptrLow; 
    uint16_t selector;  
    uint8_t ist;
    union
    {
        uint8_t flags;

        struct
        {
            uint8_t type : 4;
            uint8_t s : 1;
            uint8_t dpl : 2;
            uint8_t present : 1;
        };
    };
    uint16_t ptrMid;
    uint32_t ptrHigh;
    uint32_t reserved;
};

class IDT
{
private:
    IDTDescEntry idt[IDT_ENTRIES];
public:
    void init();
    void load();
    void registerGate(uint16_t n, uint64_t handler, uint8_t type, uint8_t dpl);
    void registerInterrupt(uint16_t n, uint64_t handler);
};

extern IDT idt;

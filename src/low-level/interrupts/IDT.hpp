#pragma once
#include <stdint.h>
#include "kernel.h"

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
    IDTDescEntry entries[IDT_ENTRIES];
    uint8_t freeVector;
    box<vector<uint8_t>> reservedVectors;
public:
    uint8_t AllocateVector();
    bool ReserveVector(uint8_t vector);
    void Init();
    void SetIST(uint8_t vector, uint8_t ist);
    void SetFlags(uint8_t vector, uint8_t flags);
    void Load();
    void RegisterGate(uint8_t vector, uint64_t handler, uint8_t type, uint8_t dpl, uint8_t ist);
    void RegisterInterrupt(uint8_t vector, uint64_t handler, uint8_t dpl = 0, uint8_t ist = 0);
};

extern IDT idt;

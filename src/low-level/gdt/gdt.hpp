#pragma once
#include "kernel.h"
#include "tss/tss.hpp"

#define GDTAccessDPL(n) (n << 5)

namespace GDTAccessFlag
{
    enum GDTAccessFlag
    {
        ReadWrite = (1 << 1),
        DC = (1 << 2),
        Execute = (1 << 3),
        Segments = (1 << 4),
        Present = (1 << 7)
    };
}

#define GDTKernelBaseSelector   0x08
#define GDTUserBaseSelector     0x18
#define GDTTSSSegment           0x30

#define GDTAccessKernelCode (GDTAccessFlag::ReadWrite | GDTAccessFlag::Execute | GDTAccessFlag::Segments | GDTAccessFlag::Present)
#define GDTAccessKernelData (GDTAccessFlag::ReadWrite | GDTAccessFlag::Segments | GDTAccessFlag::Present)
#define GDTAccessUserCode (GDTAccessFlag::ReadWrite | GDTAccessFlag::Execute | GDTAccessFlag::Segments | GDTAccessDPL(3) | GDTAccessFlag::Present)
#define GDTAccessUserData (GDTAccessFlag::ReadWrite | GDTAccessFlag::Segments | GDTAccessDPL(3) | GDTAccessFlag::Present)

struct PACKED GDTDescriptor
{
    uint16_t size;
    uint64_t offset;
};

struct PACKED GDTEntry
{
    uint16_t limitLow;
    uint16_t baseLow;
    uint8_t baseMiddle;
    uint8_t accessFlag;
    uint8_t limitFlags;
    uint8_t baseHigh;
};

struct PACKED TSSDescriptor
{
    uint16_t length;
    uint16_t baseLow;
    uint8_t baseMid;
    uint8_t flags;
    uint8_t flags2;
    uint8_t baseHigh;
    uint32_t baseUp;
    uint32_t reserved0;
};

struct PACKED __attribute__((aligned(0x1000))) GDT
{
    GDTEntry null; //0x00
    GDTEntry kernelCode; //0x08
    GDTEntry kernelData; //0x10
    GDTEntry userNull; //0x18
    GDTEntry userData; //0x20
    GDTEntry userCode; //0x28
    TSSDescriptor tss; //0x30
};

static_assert(sizeof(GDT) <= 0x1000);

extern GDT bootstrapGDT;
extern TSS bootstrapTSS;
extern GDTDescriptor bootstrapGDTR;
extern uint8_t bootstrapTssStack[0x100000];
extern uint8_t bootstrapist1Stack[0x100000];
extern uint8_t bootstrapist2Stack[0x100000];

void LoadGDT(GDT *gdt, TSS *tss, uint8_t *tssStack, uint8_t *ist1Stack, uint8_t *ist2Stack, int stackSize, GDTDescriptor *gdtr);
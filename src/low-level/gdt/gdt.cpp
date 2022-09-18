#include "gdt.hpp"
#include "debug.hpp"

uint8_t bootstrapTssStack[0x100000];
uint8_t bootstrapist1Stack[0x100000];
uint8_t bootstrapist2Stack[0x100000];

TSS bootstrapTSS = { 0 };

__attribute__((aligned(0x1000)))
GDT bootstrapGDT = {
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = 0x00,
        .limitFlags = 0x00,
        .baseHigh = 0
    }, // null
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = GDTAccessKernelCode,
        .limitFlags = 0xA0, 
        .baseHigh = 0
    }, // kernel code segment
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = GDTAccessKernelData,
        .limitFlags = 0x80,
        .baseHigh = 0
    }, // kernel data segment
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = 0x00,
        .limitFlags = 0x00,
        .baseHigh = 0
    }, // user null
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = GDTAccessUserData,
        .limitFlags = 0x80,
        .baseHigh = 0
    }, // user data segment
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = GDTAccessUserCode,
        .limitFlags = 0xA0,
        .baseHigh = 0
    }, // user code segment
    {
        .length = 104,
        .flags = 0b10001001,
    }, //TSS
};

GDTDescriptor bootstrapGDTR = {
    .size = sizeof(bootstrapGDT) - 1,
    .offset = (uint64_t)&bootstrapGDT
};

void LoadGDT(GDT *gdt, TSS *tss, uint8_t *tssStack, uint8_t *ist1Stack, uint8_t *ist2Stack, int stackSize, GDTDescriptor *gdtr)
{
    uint64_t address = (uint64_t)tss;

    gdt->tss = (TSSDescriptor) {
        .length = 104,
        .baseLow = (uint16_t)address,
        .baseMid = (uint8_t)(address >> 16),
        .flags = 0b10001001,
        .baseHigh = (uint8_t)(address >> 24),
        .baseUp = (uint32_t)(address >> 32),
    };

    tss->rsp0 = (uint64_t)tssStack + stackSize;
    tss->ist1 = (uint64_t)ist1Stack + stackSize;
    tss->ist2 = (uint64_t)ist2Stack + stackSize;

    /*
    DEBUG_OUT("%s", "GDT Offsets:");
    DEBUG_OUT("GDT Null: 0x%llx", (uint64_t)&gdt->null - (uint64_t)gdt);
    DEBUG_OUT("GDT Kernel Code: 0x%llx", (uint64_t)&gdt->kernelCode - (uint64_t)gdt);
    DEBUG_OUT("GDT Kernel Data: 0x%llx", (uint64_t)&gdt->kernelData - (uint64_t)gdt);
    DEBUG_OUT("GDT User Data: 0x%llx", (uint64_t)&gdt->userData - (uint64_t)gdt);
    DEBUG_OUT("GDT User Code: 0x%llx", (uint64_t)&gdt->userCode - (uint64_t)gdt);
    DEBUG_OUT("GDT TSS: 0x%llx", (uint64_t)&gdt->tss - (uint64_t)gdt);
    */

    asm volatile("lgdt %0" : : "m"(*gdtr));

    asm volatile("push $0x08\n"
                "lea 1f(%%rip), %%rax\n"
                "push %%rax\n"
                "lretq\n"
                "1:\n" : : : "rax", "memory");

    asm volatile("mov %0, %%ds\n"
                "mov %0, %%es\n"
                "mov %0, %%gs\n"
                "mov %0, %%fs\n"
                "mov %0, %%ss\n" : : "a"((uint16_t)0x10));

    asm volatile("ltr %0" : : "a"((uint16_t)GDTTSSSegment));
}

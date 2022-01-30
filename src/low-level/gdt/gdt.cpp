#include "gdt.hpp"
#include "tss/tss.hpp"
#include "debug.hpp"

static uint8_t tssStack[0x100000];

TSS tss = { 0 };

__attribute__((aligned(0x1000)))
GDT gdt = {
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

GDTDescriptor gdtr = {
    .size = sizeof(gdt) - 1,
    .offset = (uint64_t)&gdt
};

void LoadGDT()
{
    uint64_t address = (uint64_t)&tss;
    
    gdt.tss = (TSSDescriptor) {
        .length = 104,
        .baseLow = (uint16_t)address,
        .baseMid = (uint8_t)(address >> 16),
        .flags = 0b10001001,
        .baseHigh = (uint8_t)(address >> 24),
        .baseUp = (uint32_t)(address >> 32),
    };

    tss.rsp0 = (uint64_t)&tssStack[sizeof(tssStack)];

    DEBUG_OUT("%s", "GDT Offsets:");
    DEBUG_OUT("GDT Null: 0x%llx", (uint64_t)&gdt.null - (uint64_t)&gdt);
    DEBUG_OUT("GDT Kernel Code: 0x%llx", (uint64_t)&gdt.kernelCode - (uint64_t)&gdt);
    DEBUG_OUT("GDT Kernel Data: 0x%llx", (uint64_t)&gdt.kernelData - (uint64_t)&gdt);
    DEBUG_OUT("GDT User Data: 0x%llx", (uint64_t)&gdt.userData - (uint64_t)&gdt);
    DEBUG_OUT("GDT User Code: 0x%llx", (uint64_t)&gdt.userCode - (uint64_t)&gdt);
    DEBUG_OUT("GDT TSS: 0x%llx", (uint64_t)&gdt.tss - (uint64_t)&gdt);

    asm volatile("lgdt %0" : : "m"(gdtr));
    asm volatile("push $0x08\nlea 1f(%%rip), %%rax\npush %%rax\nlretq\n1:\n" : : : "rax", "memory");
    asm volatile("mov %0, %%ds\nmov %0, %%es\nmov %0, %%gs\nmov %0, %%fs\nmov %0, %%ss\n" : : "a"((uint16_t)0x10));
    asm volatile("ltr %0" : : "a"((uint16_t)GDTTSSSegment));
}

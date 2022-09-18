#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stivale2.h>
#include "tss/tss.hpp"
#include "gdt/gdt.hpp"
#include "schedulers/RoundRobinScheduler.hpp"

#define SMP_STACK_SIZE  0x100000
#define CPUID_XSAVE     ((uint32_t)1 << 26)
#define CPUID_AVX       ((uint32_t)1 << 28)
#define CPUID_AVX512    ((uint32_t)1 << 16)
#define CPUID_SEP       ((uint32_t)1 << 11)

struct CPUInfo
{
    int APICID;
    uint8_t *stack;
    bool bsp;
    uint32_t LAPICFrequency;
    
    GDT *gdt;
    
    GDTDescriptor gdtr;
    TSS tss;
    uint8_t tssStack[0x100000];
    uint8_t ist2Stack[0x100000];
    RoundRobinScheduler scheduler;
    void (*timerFunction)(InterruptStack *stack);

    CPUInfo() : APICID(0), stack(nullptr), bsp(false), LAPICFrequency(0), gdt(nullptr), timerFunction(nullptr) {}
};

uint32_t CPUCount();
CPUInfo *CurrentCPUInfo();
void InitializeSMP(stivale2_struct_tag_smp *smp);

void *KernelGSBase();
void *GSBase();
void SetKernelGSBase(void *address);
void SetGSBase(void *address);

static inline bool CPUID(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    uint32_t CPUIDMax;

    asm volatile (
        "cpuid"
        : "=a"(CPUIDMax)
        : "a"(leaf & 0x80000000)
        : "rbx", "rcx", "rdx"
    );

    if (leaf > CPUIDMax)
    {
        return false;
    }

    asm volatile (
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(subleaf)
    );

    return true;
}
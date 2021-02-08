#include "FPU.hpp"
#include "registers/Registers.hpp"
#include <string.h>

#define CR0_MP (1 << 1)
#define CR0_EM (1 << 2)
#define CR4_OSFXSR (1 << 9)

static uint8_t __attribute__((aligned(16))) kernelFPU[512];

FPU fpu;

void FPU::initialize()
{
    uint64_t cr0 = 0;

    asm volatile(
        "clts\n"
        "mov %%cr0, %0" : "=r"(cr0)
    );

    cr0 &= ~CR0_EM;
    cr0 |= CR0_MP;

    asm volatile("mov %0, %%cr0" : : "r"(cr0));
    asm volatile("mov %%cr4, %0" : "=r"(cr0));

    cr0 |= CR4_OSFXSR;

    asm volatile(
        "mov %0, %%cr4\n"
        "fninit" : : "r"(cr0)
    );
}

/*
void FPU::Switch(process *previous, const process *next)
{
    memcpy(previous->FPURegisters, kernelFPU, sizeof(kernelFPU));
    memcpy(kernelFPU, next->FPURegisters, sizeof(kernelFPU));
}
*/

void FPU::kernelEnter()
{
    asm volatile(
        "fxsave (%0)\n"
        "fninit\n" : : "r" (kernelFPU)
    );
}

void FPU::kernelExit()
{
    asm volatile("fxrstor (%0)\n" : : "r"(kernelFPU));
}

#include "sse.hpp"
#include "debug.hpp"
#include "../registers/Registers.hpp"
#include <cpuid.h>

bool sseEnabled = false;

#define CPUID_FEAT_ECX_AVX      1 << 28

#define CLEAR_CR0_EM(X)         ((X) & ~(1 << 2))
#define SET_CR0_MP_BIT(X)       ((X) | (1 << 1))
#define SET_CR4_OSFXSR(X)       ((X) | (1 << 9))
#define SET_CR4_OSXMMEXCPT(X)   ((X) | (1 << 10))
#define SET_CR4_OSXSAVE(X)      ((X | (1 << 18)))

void cpuid(uint32_t index, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    __get_cpuid(index, eax, ebx, ecx, edx);
}

extern "C" bool SSEEnabled()
{
    return sseEnabled;
}

extern "C" void EnableSSE()
{
    uint32_t largestExtendedFunction = 0;

    uint32_t eax, ebx, ecx, edx;

    cpuid(0x80000000, &largestExtendedFunction, &ebx, &ecx, &edx);

    if(largestExtendedFunction >= 0x80000004)
    {
        //This probably works in real hardware even tho the ptr is in the higher half? (Not sure)
        char name[48];

        cpuid(0x80000002, (uint32_t *)&name[0], (uint32_t *)&name[4], (uint32_t *)&name[8], (uint32_t *)&name[12]);

        cpuid(0x80000003, (uint32_t *)&name[16], (uint32_t *)&name[20], (uint32_t *)&name[24], (uint32_t *)&name[28]);

        cpuid(0x80000004, (uint32_t *)&name[32], (uint32_t *)&name[36], (uint32_t *)&name[40], (uint32_t *)&name[44]);

        DEBUG_OUT("CPU Name: %s", name);
    }

    DEBUG_OUT("%s", "Enabling SSE support");

    auto cr0 = Registers::ReadCR0();

    cr0 = CLEAR_CR0_EM(cr0);
    cr0 = SET_CR0_MP_BIT(cr0);

    Registers::WriteCR0(cr0);

    auto cr4 = Registers::ReadCR4();

    cr4 = SET_CR4_OSFXSR(cr4);
    cr4 = SET_CR4_OSXMMEXCPT(cr4);
    cr4 = SET_CR4_OSXSAVE(cr4);

    Registers::WriteCR4(cr4);

    sseEnabled = true;

    cpuid(0x1, &eax, &ebx, &ecx, &edx);

    uint64_t xcr0 = (1 << 0) | (1 << 1);

    if((ecx & CPUID_FEAT_ECX_AVX))
    {
        DEBUG_OUT("%s", "Enabling AVX support");

        xcr0 |= (1 << 2);
    }

    Registers::WriteXCR(0, xcr0);
}

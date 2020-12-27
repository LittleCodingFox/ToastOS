#include "registers.h"

uint64_t registerReadCR0() {

    uint64_t value = 0;

    __asm__("mov %%cr0, %0" : "=r"(value) : );

    return value;
}

uint64_t registerReadCR2() {

    uint64_t value = 0;

    __asm__("mov %%cr2, %0" : "=r"(value) : );

    return value;
}

uint64_t registerReadCR3() {

    uint64_t value = 0;

    __asm__("mov %%cr3, %0" : "=r"(value) : );

    return value;
}

uint64_t registeReadMSR(uint64_t msr) {

    uint32_t low = 0;
    uint32_t high = 0;

    __asm__("rdmsr" : "=a" (low), "=d"(high) : "c"(msr));

    return low | ((uint64_t)high << 32);
}

void registerWriteCR0(uint64_t value) {

    __asm__("mov %0, %%cr0" : : "r"(value));
}

void registerWriteCR3(uint64_t value) {

    __asm__("mov %0, %%cr3" : : "r"(value));
}

void registerWriteMSR(uint64_t msr, uint64_t value) {

    uint32_t low = (uint32_t)value;
    uint32_t high = value >> 32;

    __asm__("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}

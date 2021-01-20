#pragma once
#include <stdint.h>

class Registers
{
public:
    /**
     * Extended Feature Enables
     * @see https://wiki.osdev.org/CPU_Registers_x86-64#IA32_EFER
     */
    static const uint64_t IA32_EFER = 0xC0000080;
    /// System Call Target Address
    static const uint64_t IA32_STAR = 0xC0000081;
    /// IA-32e Mode System Call Target Address
    static const uint64_t IA32_LSTAR = 0xC0000082;
    /// System Call Flag Mask
    static const uint64_t IA32_SFMASK = 0xC0000084;

    static uint64_t readCR0();
    static uint64_t readCR2();
    static uint64_t readCR3();
    static void writeCR0(uint64_t value);
    static void writeCR3(uint64_t value);
    static uint64_t readMSR(uint64_t msr);
    static void writeMSR(uint64_t msr, uint64_t value);
    static uint64_t readRSP();
};

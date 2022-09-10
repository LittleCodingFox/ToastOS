#pragma once
#include <stdint.h>

struct RegisterState
{
    uint64_t rsp;
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
};

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

    static uint64_t ReadCR0();
    static uint64_t ReadCR2();
    static uint64_t ReadCR3();
    static uint64_t ReadCR4();
    static uint64_t ReadRFlags();
    static void WriteCR0(uint64_t value);
    static void WriteCR3(uint64_t value);
    static void WriteCR4(uint64_t value);
    static uint64_t ReadMSR(uint64_t msr);
    static void WriteMSR(uint64_t msr, uint64_t value);
    static uint64_t ReadRSP();
    static RegisterState ReadRegisters();
    static uint64_t ReadCS();
    static uint64_t ReadSS();
    static void SwapGS();
    static uint64_t ReadGS();
};

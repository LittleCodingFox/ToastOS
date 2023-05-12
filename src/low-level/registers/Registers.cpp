#include "Registers.hpp"

RegisterState Registers::ReadRegisters()
{
  RegisterState state;

  uint64_t outValue = 0;

  #define READREGISTER(name)\
    asm("mov %%" #name ", %0" : "=r"(outValue) : /* no input */);\
    state.name = outValue;

  READREGISTER(rsp);
  READREGISTER(r15);
  READREGISTER(r14);
  READREGISTER(r13);
  READREGISTER(r12);
  READREGISTER(r11);
  READREGISTER(r10);
  READREGISTER(r9);
  READREGISTER(r8);
  READREGISTER(rbp);
  READREGISTER(rdi);
  READREGISTER(rsi);
  READREGISTER(rdx);
  READREGISTER(rcx);
  READREGISTER(rbx);
  READREGISTER(rax);

  return state;
}

uint64_t Registers::ReadCS()
{
  uint64_t outValue = 0;

  asm volatile ("mov %%cs, %0" : "=r"(outValue) : /* no input */);

  return outValue;
}

uint64_t Registers::ReadSS()
{
  uint64_t outValue = 0;

  asm volatile ("mov %%ss, %0" : "=r"(outValue) : /* no input */);

  return outValue;
}

uint64_t Registers::ReadCR0()
{
  uint64_t outValue = 0;

  asm volatile ("mov %%cr0, %0" : "=r"(outValue) : /* no input */);

  return outValue;
}

uint64_t Registers::ReadRFlags()
{
  uint64_t outValue = 0;

  asm volatile ("pushf\n\t"
                "pop %0"
                : "=g"(outValue));

  return outValue;
}

uint64_t Registers::ReadRSP()
{
  uint64_t outValue = 0;

  asm volatile ("mov %%rsp, %0" : "=r"(outValue) : /* no input */);

  return outValue;
}

uint64_t Registers::ReadCR2()
{
  uint64_t outValue = 0;

  asm volatile ("mov %%cr2, %0" : "=r"(outValue) : /* no input */);

  return outValue;
}

uint64_t Registers::ReadCR3()
{
  uint64_t outValue = 0;

  asm volatile ("mov %%cr3, %0" : "=r"(outValue) : /* no input */);

  return outValue;
}

uint64_t Registers::ReadCR4()
{
  uint64_t outValue = 0;

  asm volatile ("mov %%cr4, %0" : "=r"(outValue) : /* no input */);

  return outValue;
}

void Registers::SwapGS()
{
  asm volatile ("swapgs");
}

uint64_t Registers::ReadGS()
{
  uint64_t outValue = 0;

  asm volatile ("mov %%gs:0x0, %0" : "=r" (outValue));

  return outValue;
}

void Registers::WriteCR0(uint64_t value)
{
  asm volatile ("mov %0, %%cr0" : /* no output */ : "r"(value));
}

void Registers::WriteCR3(uint64_t value)
{
  asm volatile ("mov %0, %%cr3" : /* no output */ : "r"(value));
}

void Registers::WriteCR4(uint64_t value)
{
  asm volatile ("mov %0, %%cr4" : /* no output */ : "r"(value));
}

uint64_t Registers::ReadMSR(uint64_t msr)
{
  uint32_t low = 0, high = 0;

  asm volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));

  return (low | ((uint64_t)high << 32));
}

void Registers::WriteMSR(uint64_t msr, uint64_t value)
{
  uint32_t lo = (uint32_t)value;
  uint32_t hi = value >> 32;

  asm volatile ("wrmsr" : /* no output */ : "a"(lo), "d"(hi), "c"(msr));
}

uint64_t Registers::ReadXCR(uint32_t index)
{
  uint32_t lo, hi;

  asm volatile("xgetbv" : "=a"(lo), "=d"(hi) : "c"(index) : "memory");

  return ((uint64_t)hi << 32) | (uint64_t)lo;
}

void Registers::WriteXCR(uint32_t index, uint64_t value)
{
  uint32_t lo = value;
  uint32_t hi = value >> 32;

  asm volatile("xsetbv" : : "c"(index), "a"(lo), "d"(hi) : "memory");
}

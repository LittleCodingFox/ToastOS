#include "Registers.hpp"

uint64_t Registers::readCR0()
{
  uint64_t outValue = 0;

  asm ("mov %%cr0, %0" : "=r"(outValue) : /* no input */);

  return outValue;
}

uint64_t Registers::readRSP()
{
  uint64_t outValue = 0;

  asm ("mov %%rsp, %0" : "=r"(outValue) : /* no input */);

  return outValue;
}

uint64_t Registers::readCR2()
{
  uint64_t outValue = 0;

  asm ("mov %%cr2, %0" : "=r"(outValue) : /* no input */);

  return outValue;
}

uint64_t Registers::readCR3()
{
  uint64_t outValue = 0;

  asm ("mov %%cr3, %0" : "=r"(outValue) : /* no input */);

  return outValue;
}

void Registers::writeCR0(uint64_t value)
{
  asm ("mov %0, %%cr0" : /* no output */ : "r"(value));
}

void Registers::writeCR3(uint64_t value)
{
  asm ("mov %0, %%cr3" : /* no output */ : "r"(value));
}

uint64_t Registers::readMSR(uint64_t msr)
{
  uint32_t low = 0, high = 0;

  asm("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));

  return (low | ((uint64_t)high << 32));
}

void Registers::writeMSR(uint64_t msr, uint64_t value)
{
  uint32_t lo = (uint32_t)value;
  uint32_t hi = value >> 32;

  asm ("wrmsr" : /* no output */ : "a"(lo), "d"(hi), "c"(msr));
}

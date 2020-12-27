#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

#define MSR_IA32_EFER 0xC0000080

uint64_t registerReadCR0();
uint64_t registerReadCR2();
uint64_t registerReadCR3();

uint64_t registerReadMSR(uint64_t msr);

void registerWriteCR0(uint64_t value);
void registerWriteCR3(uint64_t value);
void registerWriteMSR(uint64_t msr, uint64_t value);

#endif

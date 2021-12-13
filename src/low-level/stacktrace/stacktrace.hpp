#pragma once
#include <stdint.h>

void KernelInitStacktrace(char *symbols, size_t size);
void KernelDumpStacktrace();

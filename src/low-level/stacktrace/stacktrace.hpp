#pragma once
#include <stdint.h>

bool KernelStacktraceAvailable();
void KernelInitStacktrace(char *symbols, size_t size);
void KernelDumpStacktrace();

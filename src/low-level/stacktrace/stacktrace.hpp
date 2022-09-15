#pragma once
#include <stdint.h>
#include <stdbool.h>

#if __cplusplus
extern "C" {
#endif

bool KernelStacktraceAvailable();
void KernelInitStacktrace(char *symbols, size_t size);
void KernelDumpStacktrace();
void KernelDumpStacktraceNoLock();

#if __cplusplus
}
#endif

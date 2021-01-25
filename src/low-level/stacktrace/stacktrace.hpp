#pragma once
#include <stdint.h>

void kernelInitStacktrace(char *symbols, size_t size);
void kernelDumpStacktrace();

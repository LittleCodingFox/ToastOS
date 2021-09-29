#pragma once
#include <stddef.h>
#include <stdint.h>

void InitializeSyscalls();

bool PerformingSyscall();

void SyscallLock();

void SyscallUnlock();

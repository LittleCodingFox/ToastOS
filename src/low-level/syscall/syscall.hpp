#pragma once
#include <stddef.h>
#include <stdint.h>

#define SYSCALL_READ        1
#define SYSCALL_WRITE       2
#define SYSCALL_OPEN        3
#define SYSCALL_CLOSE       4
#define SYSCALL_SEEK        5
#define SYSCALL_ANON_ALLOC  6
#define SYSCALL_ANON_FREE   7
#define SYSCALL_VM_MAP      8
#define SYSCALL_VM_UNMAP    9

void InitializeSyscalls();

bool PerformingSyscall();

void SyscallLock();

void SyscallUnlock();

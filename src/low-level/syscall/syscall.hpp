#pragma once
#include <stddef.h>
#include <stdint.h>

#define SYSCALL_READ            1
#define SYSCALL_WRITE           2
#define SYSCALL_OPEN            3
#define SYSCALL_CLOSE           4
#define SYSCALL_SEEK            5
#define SYSCALL_ANON_ALLOC      6
#define SYSCALL_ANON_FREE       7
#define SYSCALL_VM_MAP          8
#define SYSCALL_VM_UNMAP        9
#define SYSCALL_TCB_SET         10
#define SYSCALL_SIGACTION       11
#define SYSCALL_GETPID          12
#define SYSCALL_KILL            13
#define SYSCALL_ISATTY          14
#define SYSCALL_FSTAT           15
#define SYSCALL_STAT            16
#define SYSCALL_PANIC           17
#define SYSCALL_READ_ENTRIES    18

void InitializeSyscalls();

bool PerformingSyscall();

void SyscallLock();

void SyscallUnlock();

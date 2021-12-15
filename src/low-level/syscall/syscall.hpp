#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum Syscalls
{
    SYSCALL_READ = 1,
    SYSCALL_WRITE,
    SYSCALL_OPEN,
    SYSCALL_CLOSE,
    SYSCALL_SEEK,
    SYSCALL_ANON_ALLOC,
    SYSCALL_ANON_FREE,
    SYSCALL_VM_MAP,
    SYSCALL_VM_UNMAP,
    SYSCALL_TCB_SET,
    SYSCALL_SIGACTION,
    SYSCALL_GETPID,
    SYSCALL_KILL,
    SYSCALL_ISATTY,
    SYSCALL_FSTAT,
    SYSCALL_STAT,
    SYSCALL_PANIC,
    SYSCALL_READ_ENTRIES,
    SYSCALL_EXIT,
    SYSCALL_CLOCK,
    SYSCALL_GETUID,
    SYSCALL_SETUID,
    SYSCALL_GETGID,
    SYSCALL_SIGPROCMASK,
    SYSCALL_FCNTL,
};

void InitializeSyscalls();

bool PerformingSyscall();

void SyscallLock();

void SyscallUnlock();

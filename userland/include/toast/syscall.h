#pragma once

#include <stdint.h>
#include <stdarg.h>

#if __cplusplus
extern "C"
{
#endif

#define SYSCALL_READ                1
#define SYSCALL_WRITE               2
#define SYSCALL_OPEN                3
#define SYSCALL_CLOSE               4
#define SYSCALL_SEEK                5
#define SYSCALL_ANON_ALLOC          6
#define SYSCALL_ANON_FREE           7
#define SYSCALL_VM_MAP              8
#define SYSCALL_VM_UNMAP            9
#define SYSCALL_TCB_SET             10
#define SYSCALL_SIGACTION           11
#define SYSCALL_GETPID              12
#define SYSCALL_KILL                13
#define SYSCALL_ISATTY              14
#define SYSCALL_FSTAT               15
#define SYSCALL_STAT                16
#define SYSCALL_PANIC               17
#define SYSCALL_READ_ENTRIES        18
#define SYSCALL_EXIT                19
#define SYSCALL_CLOCK               20
#define SYSCALL_GETUID              21
#define SYSCALL_SETUID              22
#define SYSCALL_GETGID              23
#define SYSCALL_SIGPROCMASK         24
#define SYSCALL_FCNTL               25
#define SYSCALL_LOG                 26
#define SYSCALL_FORK                27
#define SYSCALL_WAITPID             28
#define SYSCALL_GETPPID             29
#define SYSCALL_SETGRAPHICSTYPE     30
#define SYSCALL_GETGRAPHICSSIZE     31
#define SYSCALL_SETGRAPHICSBUFFER   32
#define SYSCALL_EXECVE              33
#define SYSCALL_POLLINPUT           34

static int64_t syscall(uint64_t id, ...)
{
    va_list args;
    va_start(args, id);

    register uint64_t rdi __asm__("rdi") = id;
    register uint64_t a1 __asm__ ("rsi") = va_arg(args, uint64_t);
    register uint64_t a2 __asm__ ("rdx") = va_arg(args, uint64_t);
    register uint64_t a3 __asm__ ("rcx") = va_arg(args, uint64_t);
    register uint64_t a4 __asm__ ("r8") = va_arg(args, uint64_t);
    register uint64_t a5 __asm__ ("r9") = va_arg(args, uint64_t);

    uint64_t s;

    asm volatile ("int $0x80"
            : "=a"(s)
            : "r"(rdi), "r"(a1), "r"(a2), "r"(a3), "r"(a4)
            : "r11", "memory");

    va_end(args);

    return s;
}

#if __cplusplus
}
#endif

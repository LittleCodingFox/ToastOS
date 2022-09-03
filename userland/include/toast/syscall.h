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
#define SYSCALL_CWD                 35
#define SYSCALL_CHDIR               36
#define SYSCALL_SLEEP               37
#define SYSCALL_SPAWN_THREAD        38
#define SYSCALL_YIELD               39
#define SYSCALL_THREAD_EXIT         40
#define SYSCALL_GET_TID             41
#define SYSCALL_FUTEX_WAIT          42
#define SYSCALL_FUTEX_WAKE          43
#define SYSCALL_SETKBLAYOUT         44
#define SYSCALL_PIPE                45
#define SYSCALL_DUP2                46
#define SYSCALL_GETRANDOM           47
#define SYSCALL_GETHOSTNAME         48
#define SYSCALL_IOCTL               49
#define SYSCALL_PSELECT             50
#define SYSCALL_TTYNAME             51
#define SYSCALL_TCGETATTR           52
#define SYSCALL_TCSETATTR           53
#define SYSCALL_TCFLOW              54
#define SYSCALL_READLINK            55
#define SYSCALL_SYSINFO             56
#define SYSCALL_GETRUSAGE           57
#define SYSCALL_GETRLIMIT           58
#define SYSCALL_FCHDIR              59
#define SYSCALL_UNAME               60

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

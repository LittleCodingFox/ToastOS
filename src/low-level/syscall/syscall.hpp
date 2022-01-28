#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

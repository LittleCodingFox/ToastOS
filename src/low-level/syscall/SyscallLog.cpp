#include "syscall.hpp"
#include "interrupts/Interrupts.hpp"
#include "printf/printf.h"
#include "debug.hpp"
#include "errno.h"

size_t SyscallLog(InterruptStack *stack)
{
    const void *buffer = (const void *)stack->rsi;
    size_t count = (size_t)stack->rdx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: log buffer: %p; count: %llu", buffer, count);
#endif

    DEBUG_OUT("%.*s", count, buffer);

    return 0;
}
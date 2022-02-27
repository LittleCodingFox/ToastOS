#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallFork(InterruptStack *stack)
{
    pid_t *child = (pid_t *)stack->rsi;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: fork child: %p", child);
#endif

    return processManager->Fork(stack, child);
}
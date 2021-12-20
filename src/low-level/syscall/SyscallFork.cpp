#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallFork(InterruptStack *stack)
{
    pid_t *child = (pid_t *)stack->rsi;

    return globalProcessManager->Fork(stack, child);
}
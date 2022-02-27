#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallFutexWake(InterruptStack *stack)
{
    int *pointer = (int *)stack->rsi;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: futex wake: pointer: %p", pointer);
#endif

    processManager->FutexWake(pointer);

    return 0;
}
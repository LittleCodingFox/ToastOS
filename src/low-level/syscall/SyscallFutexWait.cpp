#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallFutexWait(InterruptStack *stack)
{
    int *pointer = (int *)stack->rsi;
    int expected = (int)stack->rdx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: futex wait: pointer: %p; expected: %i", pointer, expected);
#endif

    processManager->FutexWait(pointer, expected, stack);

    return 0;
}
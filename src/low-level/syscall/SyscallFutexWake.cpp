#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "user/UserAccess.hpp"

int64_t SyscallFutexWake(InterruptStack *stack)
{
    int *pointer = (int *)stack->rsi;

    if(!SanitizeUserPointer(pointer))
    {
        return 0;
    }

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: futex wake: pointer: %p", pointer);
#endif

    processManager->FutexWake(pointer);

    return 0;
}
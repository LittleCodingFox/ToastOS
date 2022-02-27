#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"

int64_t SyscallExit(InterruptStack *stack)
{
    int status = stack->rsi;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: Exit status: %i", status);
#endif

    processManager->Exit(status);

    return 0;
}
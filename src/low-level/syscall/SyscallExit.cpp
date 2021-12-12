#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"

int64_t SyscallExit(InterruptStack *stack)
{
    int status = stack->rsi;

    DEBUG_OUT("Exit with status %i", status);

    globalProcessManager->Exit(status);

    return 0;
}
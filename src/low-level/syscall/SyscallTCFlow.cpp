#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "signal.h"
#include "user/UserAccess.hpp"

int64_t SyscallTCFlow(InterruptStack *stack)
{
    int fd = stack->rsi;
    int action = stack->rdx;

    (void)fd;
    (void)action;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: tcflow fd %i action %x", fd, action);
#endif

    auto process = processManager->CurrentProcess();

    if(process == NULL || process->isValid == false)
    {
        return 0;
    }

    return 0;
}

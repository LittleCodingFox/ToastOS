#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "signal.h"
#include "user/UserAccess.hpp"
#include "sys/resource.h"

int64_t SyscallGetRusage(InterruptStack *stack)
{
    int who = stack->rsi;
    struct rusage *usage = (struct rusage *)stack->rdx;

    (void)who;
    (void)usage;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: getrusage who %i; usage: %p", who, usage);
#endif

    if(!SanitizeUserPointer(usage))
    {
        return -EINVAL;
    }

    return 0;
}

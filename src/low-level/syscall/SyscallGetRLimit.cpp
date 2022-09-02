#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "signal.h"
#include "user/UserAccess.hpp"
#include "resource.h"

int64_t SyscallGetRLimit(InterruptStack *stack)
{
    int resource = stack->rsi;
    struct rlimit *limit = (struct rlimit *)stack->rdx;

    (void)resource;
    (void)limit;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: resource %i; limit: %p", resource, limit);
#endif

    if(!SanitizeUserPointer(limit))
    {
        return -EINVAL;
    }

    return 0;
}

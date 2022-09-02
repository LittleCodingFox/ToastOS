#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "signal.h"
#include "user/UserAccess.hpp"

int64_t SyscallSysInfo(InterruptStack *stack)
{
    struct sysinfo *sysinfo = (struct sysinfo *)stack->rsi;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: sysinfo %p", sysinfo);
#endif

    if(!SanitizeUserPointer(sysinfo))
    {
        return -EINVAL;
    }

    return 0;
}

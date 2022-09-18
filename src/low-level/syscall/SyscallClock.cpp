#include <time.h>
#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "cmos/cmos.hpp"
#include "time/time.hpp"
#include "user/UserAccess.hpp"

int64_t SyscallClock(InterruptStack *stack)
{
    int clock = (int)stack->rsi;
    time_t *secs = (time_t *)stack->rdx;
    long *nanos = (long *)stack->rcx;

    (void)clock;
    (void)secs;
    (void)nanos;

    if(!SanitizeUserPointer(secs) || !SanitizeUserPointer(nanos))
    {
        return 0;
    }

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: Clock clock: %i secs: %p nanos: %p", clock, secs, nanos);
#endif

    if(clock == CLOCK_MONOTONIC || clock == CLOCK_MONOTONIC_RAW)
    {
        *secs = monotonicTime.tv_sec;
        *nanos = monotonicTime.tv_nsec;
    }
    else if(clock == CLOCK_REALTIME)
    {
        *secs = realtimeTime.tv_sec;
        *nanos = realtimeTime.tv_nsec;
    }

    //TODO

    return 0;
}
#include <time.h>
#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "cmos/cmos.hpp"
#include "pit/PIT.hpp"
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

    /*
    auto ms = PITGetCurrentCount() * (1000 / PIT_DIVIDEND);

    if(clock == CLOCK_MONOTONIC || clock == CLOCK_MONOTONIC_RAW)
    {
        *secs = ms / 1000;
        *nanos = (ms % 1000) * 1000000;
    }
    else if(clock == CLOCK_REALTIME)
    {
        *secs = cmos.CurrentTime();
        *nanos = *secs * 1000000000 + (ms % 1000) * 1000000;
    }
    */

    //TODO

    return 0;
}
#include <time.h>
#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "cmos/cmos.hpp"

int64_t SyscallClock(InterruptStack *stack)
{
    int clock = (int)stack->rsi;
    time_t *secs = (time_t *)stack->rdx;
    long *nanos = (long *)stack->rcx;

    if(clock == CLOCK_MONOTONIC || clock == CLOCK_MONOTONIC_RAW)
    {
        *secs = cmos.CurrentTime();
        *nanos = *secs * 1000000000;
    }
    else if(clock == CLOCK_REALTIME)
    {
        *secs = cmos.CurrentTime();
        *nanos = *secs * 1000000000;
    }

    return 0;
}
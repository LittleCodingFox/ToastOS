#include <time.h>
#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "cmos/cmos.hpp"
#include "timer/Timer.hpp"

int64_t SyscallClock(InterruptStack *stack)
{
    int clock = (int)stack->rsi;
    time_t *secs = (time_t *)stack->rdx;
    long *nanos = (long *)stack->rcx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: Clock clock: %i secs: %p nanos: %p", clock, secs, nanos);
#endif

    auto ms = timer->GetTicks() * (1000 / timer->Frequency());

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

    return 0;
}
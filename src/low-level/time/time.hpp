#include <stddef.h>
#include <stdint.h>
#include "time.h"

#define TIMER_FREQ 100

struct Timer
{
    uint64_t index;
    bool fired;
    timespec when;
};

extern timespec monotonicTime;
extern timespec realtimeTime;

static inline timespec TimespecAdd(struct timespec a, struct timespec b)
{
    if (a.tv_nsec + b.tv_nsec > 999999999)
    {
        a.tv_nsec = (a.tv_nsec + b.tv_nsec) - 1000000000;
        a.tv_sec++;
    }
    else
    {
        a.tv_nsec += b.tv_nsec;
    }

    a.tv_sec += b.tv_sec;

    return a;
}

static inline timespec TimespecSubtract(struct timespec a, struct timespec b)
{
    if (b.tv_nsec > a.tv_nsec)
    {
        a.tv_nsec = 999999999 - (b.tv_nsec - a.tv_nsec);

        if (a.tv_sec > 0)
        {
            a.tv_sec--;
        }
        else
        {
            a.tv_sec = a.tv_nsec = 0;
        }
    }
    else
    {
        a.tv_nsec -= b.tv_nsec;
    }

    if (b.tv_sec > a.tv_sec)
    {
        a.tv_sec = a.tv_nsec = 0;
    }
    else
    {
        a.tv_sec -= b.tv_sec;
    }

    return a;
}

Timer *CreateTimer(timespec when);
void ArmTimer(Timer *timer);
void DisarmTimer(Timer *timer);

void InitializeTime();

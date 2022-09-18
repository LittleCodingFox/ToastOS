#include "time.hpp"
#include "kernel.h"
#include "cmos/cmos.hpp"
#include "threading/lock.hpp"
#include "pit/PIT.hpp"

timespec monotonicTime = {0, 0};
timespec realtimeTime = {0, 0};

static AtomicLock timeLock;
box<vector<Timer *>> armedTimers;

Timer *NewTimer(timespec when)
{
    Timer *timer = new Timer();

    timer->when = when;

    ArmTimer(timer);

    return timer;
}

void ArmTimer(Timer *timer)
{
    timeLock.Lock();

    timer->index = armedTimers.get()->size();
    timer->fired = false;

    armedTimers->push_back(timer);

    timeLock.Unlock();
}

void DisarmTimer(Timer *timer)
{
    timeLock.Lock();

    if(armedTimers->size() == 0 || timer->index == -1 || timer->index >= armedTimers->size())
    {
        timeLock.Unlock();

        return;
    }

    //Vector doesn't support erasure
    vector<Timer *> newTimers;

    for(auto &t : *armedTimers.get())
    {
        if(t == timer)
        {
            continue;
        }

        newTimers.push_back(t);
    }

    *armedTimers.get() = newTimers;

    timer->index = -1;

    timeLock.Unlock();
}

void InitializeTime()
{
    armedTimers.initialize();
    
    realtimeTime.tv_sec = cmos.BootTime();

    InitializePIT();
}

void TimerHandler()
{
    timespec interval = {
        .tv_sec = 0,
        .tv_nsec = 1000000000 / TIMER_FREQ
    };

    monotonicTime = TimespecAdd(monotonicTime, interval);
    realtimeTime = TimespecAdd(realtimeTime, interval);

    if(timeLock.IsLocked())
    {
        return;
    }

    timeLock.Lock();

    for(uint64_t i = 0; i < armedTimers->size(); i++)
    {
        Timer *timer = (*armedTimers.get())[i];

        if(timer->fired)
        {
            continue;
        }

        timer->when = TimespecSubtract(timer->when, interval);

        if(timer->when.tv_sec == 0 && timer->when.tv_nsec == 0)
        {
            //TODO: Trigger event

            timer->fired = true;
        }
    }

    timeLock.Unlock();
}

#include "Timer.hpp"
#include "ports/Ports.hpp"
#include "debug.hpp"
#include <stdlib.h>

#define TIMER_FREQUENCY 200 // in Hz
#define TIMER_QUOTIENT  1193180

#define PIT_0           0x40
#define PIT_1           0x41
#define PIT_2           0x42
#define PIT_CMD         0x43
#define PIT_SET         0x36

static uint64_t currentTick;

frg::manual_box<Timer> timer;

void timerCallback(InterruptStack *stack)
{
    currentTick++;

    if(timer)
    {
        timer->RunHandlers(stack);
    }
}

void Timer::Initialize()
{
    interrupts.RegisterHandler(IRQ0, timerCallback);

    uint32_t divisor = TIMER_QUOTIENT / TIMER_FREQUENCY;

    outport8(PIT_CMD, PIT_SET);
    outport8(PIT_0, divisor & 0xFF);
    outport8(PIT_0, (divisor >> 8) & 0xFF);
}

uint32_t Timer::Frequency()
{
    return TIMER_FREQUENCY;
}

void Timer::RunHandlers(InterruptStack *stack)
{
    for(uint32_t i = 0; i < handlers.size(); i++)
    {
        void (*handler)(InterruptStack *) = (void (*)(InterruptStack *))handlers[i];

        handler(stack);
    }
}

void Timer::RegisterHandler(void (*callback)(InterruptStack *))
{
    handlers.push_back((void *)callback);
}

void Timer::UnregisterHandler(void (*callback)(InterruptStack *))
{
    //TODO: Remove
    //handlers.remove((void *)callback);
}

uint64_t Timer::GetTicks()
{
    return currentTick;
}

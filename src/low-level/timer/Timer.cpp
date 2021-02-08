#include "Timer.hpp"
#include "ports/Ports.hpp"
#include "debug.hpp"
#include <stdlib.h>

#define TIMER_FREQUENCY 50 // in Hz
#define TIMER_QUOTIENT  1193180

#define PIT_0           0x40
#define PIT_1           0x41
#define PIT_2           0x42
#define PIT_CMD         0x43
#define PIT_SET         0x36

static uint32_t currentTick;

Timer timer;

void timerCallback(InterruptStack *stack)
{
    currentTick++;

    timer.runHandlers();
}

void Timer::initialize()
{
    handlers = (DynamicArray<void *> *)malloc(sizeof(DynamicArray<void *>));

    interrupts.registerHandler(IRQ0, timerCallback);

    uint32_t divisor = TIMER_QUOTIENT / TIMER_FREQUENCY;

    outport8(PIT_CMD, PIT_SET);
    outport8(PIT_0, divisor & 0xFF);
    outport8(PIT_0, (divisor >> 8) & 0xFF);
}

void Timer::runHandlers()
{
    for(uint32_t i = 0; i < handlers->length(); i++)
    {
        void (*handler)() = (void (*)())(*handlers)[i];

        handler();
    }
}

void Timer::registerHandler(void (*callback)())
{
    handlers->add((void *)callback);
}

void Timer::unregisterHandler(void (*callback)())
{
    handlers->remove((void *)callback);
}

uint32_t Timer::getTicks()
{
    return currentTick;
}

float Timer::getTime()
{
    return currentTick * (1.0f / TIMER_FREQUENCY);
}

#pragma once

#include "interrupts/Interrupts.hpp"
#include "dynamicarray.hpp"

class Timer
{
private:
    DynamicArray<void *> *handlers;
public:
    void Initialize();
    uint32_t GetTicks();
    void RegisterHandler(void (*callback)(InterruptStack *));
    void UnregisterHandler(void (*callback)(InterruptStack *));
    void RunHandlers(InterruptStack *stack);
};

extern Timer timer;

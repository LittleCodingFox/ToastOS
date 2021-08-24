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
    float GetTime();
    void RegisterHandler(void (*callback)());
    void UnregisterHandler(void (*callback)());
    void RunHandlers();
};

extern Timer timer;

void Sleep(uint64_t milliseconds);

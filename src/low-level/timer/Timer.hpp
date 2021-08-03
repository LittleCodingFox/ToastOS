#pragma once

#include "interrupts/Interrupts.hpp"
#include "klibc/dynamicarray.hpp"

class Timer
{
private:
    DynamicArray<void *> *handlers;
public:
    void initialize();
    uint32_t getTicks();
    float getTime();
    void registerHandler(void (*callback)());
    void unregisterHandler(void (*callback)());
    void runHandlers();
};

extern Timer timer;

void Sleep(uint64_t milliseconds);

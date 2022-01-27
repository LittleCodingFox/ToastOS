#pragma once

#include <frg/vector.hpp>
#include <frg/manual_box.hpp>
#include <frg_allocator.hpp>

#include "interrupts/Interrupts.hpp"

class Timer
{
private:
    frg::vector<void *, frg_allocator> handlers;
public:
    void Initialize();
    uint32_t Frequency();
    uint64_t GetTicks();
    void RegisterHandler(void (*callback)(InterruptStack *));
    void UnregisterHandler(void (*callback)(InterruptStack *));
    void RunHandlers(InterruptStack *stack);
};

extern frg::manual_box<Timer> timer;

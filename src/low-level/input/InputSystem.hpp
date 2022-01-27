#pragma once

#include <stdint.h>
#include <stddef.h>
#include <kernel.h>
#include "../../../userland/include/toast/input.h"

class InputSystem
{
private:
    vector<InputEvent> events;
    Threading::AtomicLock lock;
public:
    void AddEvent(const InputEvent &event);
    bool Poll(InputEvent *event);
};

extern box<InputSystem> globalInputSystem;

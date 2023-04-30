#pragma once

#include <stdint.h>
#include <stddef.h>
#include <kernel.h>
#include "../../../userland/include/toast/input.h"
#include "../threading/lock.hpp"

class InputSystem
{
private:
    vector<InputEvent> events;
    AtomicLock lock;
public:
    void AddEvent(const InputEvent &event);
    bool Poll(InputEvent *event);
};

extern box<InputSystem> globalInputSystem;

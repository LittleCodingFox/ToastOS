#include "InputSystem.hpp"

box<InputSystem> globalInputSystem;

void InputSystem::AddEvent(const InputEvent &event)
{
    Threading::ScopedLock lock(this->lock);

    events.push_back(event);
}

bool InputSystem::Poll(InputEvent *event)
{
    Threading::ScopedLock lock(this->lock);

    if(event == NULL || events.size() == 0)
    {
        return false;
    }

    *event = events[0];

    //Very ineffective, but will do for now
    vector<InputEvent> newEvents;

    for(size_t i = 1; i < events.size(); i++)
    {
        newEvents.push_back(events[i]);
    }

    events = newEvents;

    return true;
}

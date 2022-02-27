#pragma once

#include <stddef.h>
#include <stdint.h>

class AtomicLock
{
private:
    volatile uint32_t locked;
public:
    AtomicLock();

    bool IsLocked() const;

    void Lock();
    void Unlock();

    void ForceLock();
};

class ScopedLock
{
private:
    AtomicLock &lock;
public:
    ScopedLock(AtomicLock &value);
    ~ScopedLock();
};

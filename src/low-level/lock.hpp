#pragma once

#include <stddef.h>
#include <stdint.h>
#include "registers/Registers.hpp"
#include "interrupts/Interrupts.hpp"

class AtomicLock
{
private:
    uint32_t locked;
public:
    AtomicLock() : locked(0) {};

    bool IsLocked() const
    {
        uint32_t result = 0;

        __atomic_load(&locked, &result, __ATOMIC_SEQ_CST);

        return result;
    }

    void Lock()
    {
        uint32_t expected = false;
        uint32_t desired = true;

        while(!__atomic_compare_exchange(&locked, &expected, &desired, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        {
            expected = false;

            asm volatile("pause");
        }
    }

    void ForceLock()
    {
        locked = true;
    }

    void Unlock()
    {
        __atomic_store_n(&locked, false, __ATOMIC_RELEASE);

        locked = false;
    }
};

class ScopedLock
{
private:
    AtomicLock &lock;
public:
    ScopedLock(AtomicLock &value) : lock(value)
    {
        lock.Lock();
    }

    ~ScopedLock()
    {
        lock.Unlock();
    }
};

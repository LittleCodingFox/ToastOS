#include "lock.hpp"

AtomicLock::AtomicLock() : locked(0) {};

bool AtomicLock::IsLocked() const
{
    uint32_t result = 0;

    __atomic_load(&locked, &result, __ATOMIC_SEQ_CST);

    return result;
}

void AtomicLock::Lock()
{
    uint32_t expected = false;
    uint32_t desired = true;

    while(!__atomic_compare_exchange(&locked, &expected, &desired, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
    {
        expected = false;

        asm volatile("pause");
    }
}

void AtomicLock::ForceLock()
{
    locked = true;
}

void AtomicLock::Unlock()
{
    __atomic_store_n(&locked, false, __ATOMIC_RELEASE);

    locked = false;
}

ScopedLock::ScopedLock(AtomicLock &value) : lock(value)
{
    lock.Lock();
}

ScopedLock::~ScopedLock()
{
    lock.Unlock();
}

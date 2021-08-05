#pragma once

#include <stddef.h>
#include <stdint.h>

namespace Threading
{
    class Lock
    {
    private:
        bool locked;
    public:
        Lock() : locked(false) {};

        bool IsLocked() const
        {
            bool result = false;

            __atomic_load(&locked, &result, __ATOMIC_SEQ_CST);

            return result;
        }

        void Lock()
        {
            while(!__sync_bool_compare_and_swap(&locked, false, true))
            {
                asm volatile("pause");
            }

            __sync_synchronize();
        }

        void ForceLock()
        {
            locked = true;

            __sync_synchronize();
        }

        void Unlock()
        {
            __sync_synchronize();
            __atomic_store_n(&locked, false, __ATOMIC_SEQ_CST);

            locked = false;
        }
    };

    class ScopedLock
    {
    private:
        Lock &lock;
    public:
        ScopedLock(Lock &value) : lock(value)
        {
            lock.Lock();
        }

        ~ScopedLock()
        {
            lock.Unlock();
        }
    };
}

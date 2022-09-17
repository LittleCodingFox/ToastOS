#include <stdint.h>
#include <stdlib.h>
#include "threading/lock.hpp"

AtomicLock memoryLock;

void* operator new(size_t size)
{
    ScopedLock lock(memoryLock);

    return malloc(size);
}

void* operator new[](size_t size)
{
    ScopedLock lock(memoryLock);

    return malloc(size);
}

void operator delete(void* p)
{
    ScopedLock lock(memoryLock);

    free(p);
}

void operator delete(void *p, uint64_t length)
{
    ScopedLock lock(memoryLock);

    free(p);
}

void operator delete[](void *p)
{
    ScopedLock lock(memoryLock);

    free(p);
}

#include "UserAccess.hpp"
#include "paging/Paging.hpp"

bool ReadUserMemory(void *kernelPtr, const void *userPtr, size_t size)
{
    uintptr_t limit;

    if(__builtin_add_overflow((uintptr_t)userPtr, size, &limit))
    {
        return false;
    }

    if(IsHigherHalf(limit))
    {
        return false;
    }

    //TODO

    return false;
}

bool WriteUserMemory(void *userPtr, const void *kernelPtr, size_t size)
{
    uintptr_t limit;

    if(__builtin_add_overflow((uintptr_t)userPtr, size, &limit))
    {
        return false;
    }

    if(IsHigherHalf(limit))
    {
        return false;
    }

    //TODO

    return false;
}

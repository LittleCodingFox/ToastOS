#include "UserAccess.hpp"
#include "paging/Paging.hpp"

bool [[gnu::no_sanitize_address]] ReadUserMemory(void *kernelPtr, const void *userPtr, size_t size)
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

    memcpy(kernelPtr, userPtr, size);

    return false;
}

bool [[gnu::no_sanitize_address]] WriteUserMemory(void *userPtr, const void *kernelPtr, size_t size)
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

    memcpy(userPtr, kernelPtr, size);

    return false;
}

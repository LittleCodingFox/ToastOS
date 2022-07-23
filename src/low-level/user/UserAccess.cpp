#include "UserAccess.hpp"
#include "paging/Paging.hpp"

[[gnu::no_sanitize_address]] bool SanitizeUserPointer(const void *userPtr)
{
    if(IsHigherHalf((uint64_t)userPtr))
    {
        return false;
    }

    if(userPtr == NULL)
    {
        return false;
    }

    return true;
}

[[gnu::no_sanitize_address]] bool ReadUserMemory(void *kernelPtr, const void *userPtr, size_t size)
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

    return true;
}

[[gnu::no_sanitize_address]] bool WriteUserMemory(void *userPtr, const void *kernelPtr, size_t size)
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

    return true;
}

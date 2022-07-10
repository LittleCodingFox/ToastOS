#pragma once

#include "kernel.h"

struct UserAccessRegion {

    void *startIP;
    void *endIP;
    void *faultIP;
    uint32_t flags;
};

bool ReadUserMemory(void *kernelPtr, const void *userPtr, size_t size);

bool WriteUserMemory(void *userPtr, const void *kernelPtr, size_t size);

template<typename T>
bool ReadUserObject(const T *pointer, T &object)
{
    return ReadUserMemory(&object, pointer, sizeof(T));
}

template<typename T>
bool WriteUserObject(T *pointer, T object)
{
    return WriteUserMemory(pointer, &object, sizeof(T));
}

template<typename T>
bool ReadUserArray(const T *pointer, T *array, size_t count)
{
    size_t size;

    if(__builtin_mul_overflow(sizeof(T), count, &size))
    {
        return false;
    }

    return ReadUserMemory(array, pointer, size);
}

template<typename T>
bool WriteUserObject(T *pointer, const T *array, size_t count)
{
    size_t size;

    if(__builtin_mul_overflow(sizeof(T), count, &size))
    {
        return false;
    }

    return WriteUserMemory(pointer, array, size);
}

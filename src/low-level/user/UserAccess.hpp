#pragma once

#include "kernel.h"

[[gnu::no_sanitize_address]] bool SanitizeUserPointer(const void *userPtr);

[[gnu::no_sanitize_address]] bool ReadUserMemory(void *kernelPtr, const void *userPtr, size_t size);

[[gnu::no_sanitize_address]] bool WriteUserMemory(void *userPtr, const void *kernelPtr, size_t size);

template<typename T>
[[gnu::no_sanitize_address]] bool ReadUserObject(const T *pointer, T &object)
{
    return ReadUserMemory(&object, pointer, sizeof(T));
}

template<typename T>
[[gnu::no_sanitize_address]] bool WriteUserObject(T *pointer, T object)
{
    return WriteUserMemory(pointer, &object, sizeof(T));
}

template<typename T>
[[gnu::no_sanitize_address]] bool ReadUserArray(const T *pointer, T *array, size_t count)
{
    size_t size;

    if(__builtin_mul_overflow(sizeof(T), count, &size))
    {
        return false;
    }

    return ReadUserMemory(array, pointer, size);
}

template<typename T>
[[gnu::no_sanitize_address]] bool WriteUserArray(T *pointer, const T *array, size_t count)
{
    size_t size;

    if(__builtin_mul_overflow(sizeof(T), count, &size))
    {
        return false;
    }

    return WriteUserMemory(pointer, array, size);
}

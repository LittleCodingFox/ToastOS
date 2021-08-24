#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

template<typename T>
class DynamicArray
{
private:
    uint32_t size;
    T *ptr;
public:
    DynamicArray() : size(0), ptr(NULL) {}
    DynamicArray(const DynamicArray<T> &other) : size(other.size)
    {
        ptr = new T[size];

        for(uint32_t i = 0; i < size; i++)
        {
            ptr[i] = other.ptr[i];
        }
    }

    DynamicArray(uint32_t size) : size(size)
    {
        if(size > 0)
        {
            ptr = new T[size];
        }
    }

    void add(T element)
    {
        T *newPtr = new T[size + 1];

        if(ptr != NULL)
        {
            for(uint32_t i = 0; i < size; i++)
            {
                newPtr[i] = ptr[i];
            }
        }

        newPtr[size] = element;

        delete [] ptr;
        ptr = newPtr;
        size++;
    }

    void remove(int index)
    {
        if(index < 0 || index >= size || size == 0)
        {
            return;
        }

        if(size == 1)
        {
            delete[] ptr;
            ptr = NULL;
            size = 0;
        }

        T *newPtr = new T[size - 1];

        T *dst = newPtr;

        for(uint32_t i = 0; i < size; i++)
        {
            if(i == index)
            {
                continue;
            }

            *dst++ = ptr[i];
        }

        delete [] ptr;

        ptr = newPtr;
        size--;
    }

    void remove(T element)
    {
        for(int i = 0; i < size; i++)
        {
            if(ptr[i] == element)
            {
                remove(i);

                return;
            }
        }
    }

    const T &operator[](int index) const
    {
        return ptr[index];
    }

    T &operator[](int index)
    {
        return ptr[index];
    }

    int length() const
    {
        return size;
    } 
};

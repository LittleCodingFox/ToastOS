#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

template<typename T>
class DynamicArray
{
private:
    T *ptr;
    uint32_t size;
public:
    void initialize(uint32_t size)
    {
        if(size > 0)
        {
            ptr = (T *)malloc(sizeof(T) * size);
        }

        this->size = size;
    }

    void add(T element)
    {
        T *newPtr = (T *)malloc(sizeof(T) * (size + 1));

        if(ptr != NULL)
        {
            memcpy(newPtr, ptr, sizeof(T) * size);
        }

        newPtr[size] = element;

        free(ptr);
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
            free(ptr);
            ptr = NULL;
            size = 0;
        }

        T *newPtr = (T *)malloc(sizeof(T) * (size - 1));

        T *dst = newPtr;

        for(uint32_t i = 0; i < size; i++)
        {
            if(i == index)
            {
                continue;
            }

            *dst++ = ptr[i];
        }

        free(ptr);

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

    T &operator[](int index)
    {
        return ptr[index];
    }

    int length() const
    {
        return size;
    } 
};


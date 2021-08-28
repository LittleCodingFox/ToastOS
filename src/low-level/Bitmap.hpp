#pragma once
#include <stddef.h>
#include <stdint.h>
#include "debug.hpp"

class Bitmap
{
public:
    size_t size;
    uint8_t* buffer;

    inline bool operator[](uint64_t index)
    {
        if (index > size * 8)
        {
            return false;
        }

        uint64_t byteIndex = index / 8;
        uint8_t bitIndex = index % 8;
        uint8_t bitIndexer = 0b10000000 >> bitIndex;
        
        return ((buffer[byteIndex] & bitIndexer) > 0);
    }

    inline bool Set(uint64_t index, bool value)
    {
        if (index > size * 8)
        {
            return false;
        }

        uint64_t byteIndex = index / 8;
        uint8_t bitIndex = index % 8;
        uint8_t bitIndexer = 0b10000000 >> bitIndex;

        buffer[byteIndex] &= ~bitIndexer;

        if (value)
        {
            buffer[byteIndex] |= bitIndexer;
        }

        return true;
    }
};

inline uint8_t GetBit(uint8_t *bitmap, size_t bit)
{
    size_t bitmapIndex = bit / 8;
    size_t bitIndex = bit % 8;

    return bitmap[bitmapIndex] & (1 << bitIndex);
}

inline void SetBit(uint8_t *bitmap, size_t bit, uint8_t value)
{
    size_t bitmapIndex = bit / 8;
    size_t bitIndex = bit % 8;

    if(value)
    {
        bitmap[bitmapIndex] |= (1 << bitIndex);
    }
    else
    {
        bitmap[bitmapIndex] &= ~(1 << bitIndex);
    }
}

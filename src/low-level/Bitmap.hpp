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

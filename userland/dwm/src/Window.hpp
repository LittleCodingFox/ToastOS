#pragma once

#include <stdint.h>
#include <stddef.h>
#include "frgdef.hpp"

struct Window
{
    string title;
    float x, y, width, height;

    Window() : x(0), y(0), width(0), height(0)
    {
    }
};

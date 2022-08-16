#pragma once
#include <stdint.h>
#include <stddef.h>

extern int screenWidth;
extern int screenHeight;

namespace ScreenType
{
    enum ScreenType
    {
        Logo,
        Main,
        Shutdown
    };
}

struct Screen
{
    uint32_t type;
    void (*draw)(float delta);
};

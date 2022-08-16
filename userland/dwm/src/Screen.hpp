#pragma once
#include <stdint.h>
#include <stddef.h>

extern int screenWidth;
extern int screenHeight;
extern int mouseX;
extern int mouseY;
extern int mouseButtons;

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

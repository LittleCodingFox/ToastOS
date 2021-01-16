#pragma once
#include "mathstructs.h"
#include "Framebuffer.hpp"
#include <text/psf2.hpp>
#include <stdint.h>

class BasicRenderer
{
private:

    Framebuffer* TargetFramebuffer;
    psf2_font_t* font;

public:

    Point CursorPosition;
    unsigned int Colour;

    BasicRenderer(Framebuffer* targetFramebuffer, psf2_font_t* font);
    void Print(const char* str);
    void PutChar(char chr, unsigned int xOff, unsigned int yOff);
    void Clear(uint32_t colour);
    void Newline();
    uint32_t *FrameBufferPtrAt(int x, int y);
};

extern BasicRenderer* GlobalRenderer;

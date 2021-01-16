#pragma once
#include "mathstructs.h"
#include "Framebuffer.hpp"
#include <text/psf2.hpp>
#include <stdint.h>

class FramebufferRenderer
{
private:

    Framebuffer* TargetFramebuffer;
    psf2_font_t* font;

public:

    Point CursorPosition;
    unsigned int Colour;

    FramebufferRenderer(Framebuffer* targetFramebuffer, psf2_font_t* font);
    void Print(const char* str);
    void PutChar(char chr, unsigned int xOff, unsigned int yOff);
    void PutChar(char chr);
    void Clear(uint32_t colour);
    void Newline();
    void ClearChar();
    uint32_t *FrameBufferPtrAt(int x, int y);
};

extern FramebufferRenderer* GlobalRenderer;

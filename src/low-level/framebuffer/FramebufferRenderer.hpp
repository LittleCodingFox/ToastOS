#pragma once
#include "mathstructs.h"
#include "Framebuffer.hpp"
#include <text/psf2.hpp>
#include <stdint.h>

struct FramebufferArea
{
    int x;
    int y;
    int width;
    int height;
    int targetX;
    int targetY;
    int targetWidth;
    int targetHeight;
    uint32_t *buffer;

    inline bool IsValid() const
    {
        return x >= 0 && y >= 0 && width > 0 && height > 0 &&
            targetX >= 0 && targetY >= 0 && targetWidth > 0 && targetHeight > 0 &&
            buffer != NULL;
    }
};

union Color32
{
    struct
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

    uint32_t value;

    operator uint32_t() const
    {
        return value;
    }
};

class FramebufferRenderer
{
private:

    Framebuffer *targetFramebuffer;
    psf2_font_t *font;
    uint32_t *doubleBuffer;
    bool initialized;

public:

    Point cursorPosition;
    uint32_t colour;

    FramebufferRenderer(Framebuffer* targetFramebuffer, psf2_font_t* font);
    void initialize();
    void clear(uint32_t color);
    void putChar(char chr, unsigned int xOff, unsigned int yOff);
    FramebufferArea frameBufferAreaAt(int x, int y, int width, int height);
    void swapBuffers();

    inline int width() const
    {
        if(targetFramebuffer != NULL)
        {
            return targetFramebuffer->width;
        }

        return 0;
    }

    inline int height() const
    {
        if(targetFramebuffer != NULL)
        {
            return targetFramebuffer->height;
        }

        return 0;
    }

    inline int fontWidth() const
    {
        if(font != NULL)
        {
            return font->header->width;
        }

        return 0;
    }

    inline int fontHeight() const
    {
        if(font != NULL)
        {
            return font->header->height;
        }

        return 0;
    }

    inline psf2_font_t *getFont() const
    {
        return font;
    }
};

extern FramebufferRenderer* globalRenderer;

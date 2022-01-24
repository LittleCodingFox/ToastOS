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

enum 
{
    GRAPHICS_TYPE_CONSOLE,
    GRAPHICS_TYPE_UI,
};

class FramebufferRenderer
{
private:

    uint32_t lockedCount;
    Framebuffer *targetFramebuffer;
    psf2_font_t *font;
    uint32_t *doubleBuffer;
    bool initialized;
    size_t doubleBufferSize;
    size_t framebufferPixelCount;
    uint32_t graphicsType;

public:

    uint32_t backgroundColor;

    FramebufferRenderer(Framebuffer* targetFramebuffer, psf2_font_t* font);
    void Initialize();
    void Clear(uint32_t color);
    void PutChar(char chr, unsigned int xOff, unsigned int yOff, uint32_t color = 0xFFFFFFFF);
    FramebufferArea FrameBufferAreaAt(int x, int y, int width, int height);
    void SwapBuffers();
    void Lock();
    void Unlock();
    void SetGraphicsType(int graphicsType);
    void SetGraphicsBuffer(const void *buffer);

    inline int Width() const
    {
        if(targetFramebuffer != NULL)
        {
            return targetFramebuffer->width;
        }

        return 0;
    }

    inline int Height() const
    {
        if(targetFramebuffer != NULL)
        {
            return targetFramebuffer->height;
        }

        return 0;
    }

    inline int BitsPerPixel() const
    {
        if(targetFramebuffer != NULL)
        {
            return sizeof(uint32_t);
        }

        return 0;
    }

    inline int FontWidth() const
    {
        if(font != NULL)
        {
            return font->header->width;
        }

        return 0;
    }

    inline int FontHeight() const
    {
        if(font != NULL)
        {
            return font->header->height;
        }

        return 0;
    }

    inline psf2_font_t *GetFont() const
    {
        return font;
    }
};

extern FramebufferRenderer* globalRenderer;

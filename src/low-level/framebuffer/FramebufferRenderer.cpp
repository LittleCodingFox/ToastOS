#include "FramebufferRenderer.hpp"
#include "debug.hpp"

FramebufferRenderer* globalRenderer;

FramebufferRenderer::FramebufferRenderer(Framebuffer* targetFramebuffer, psf2_font_t* font)
{
    this->targetFramebuffer = targetFramebuffer;
    this->font = font;

    colour = 0xffffffff;
    cursorPosition = {0, 0};
}

void FramebufferRenderer::Clear(uint32_t colour)
{
    uint64_t fbBase = (uint64_t)targetFramebuffer->baseAddress;
    uint64_t bytesPerScanline = targetFramebuffer->pixelsPerScanLine * 4;
    uint64_t fbHeight = targetFramebuffer->height;

    for (int verticalScanline = 0; verticalScanline < fbHeight; verticalScanline ++)
    {
        uint64_t pixPtrBase = fbBase + (bytesPerScanline * verticalScanline);

        for (uint32_t* pixPtr = (uint32_t*)pixPtrBase; pixPtr < (uint32_t*)(pixPtrBase + bytesPerScanline); pixPtr ++)
        {
            *pixPtr = colour;
        }
    }
}

void FramebufferRenderer::ClearChar()
{
    if (cursorPosition.x == 0)
    {
        cursorPosition.x = targetFramebuffer->width;
        cursorPosition.y -= font->header->height;

        if (cursorPosition.y < 0) cursorPosition.y = 0;
    }

    unsigned int xOff = cursorPosition.x;
    unsigned int yOff = cursorPosition.y;

    unsigned int* pixPtr = (unsigned int*)targetFramebuffer->baseAddress;
    for (unsigned long y = yOff; y < yOff + font->header->height; y++)
    {
        for (unsigned long x = xOff - font->header->width; x < xOff; x++)
        {
            *(unsigned int*)(pixPtr + x + (y * targetFramebuffer->pixelsPerScanLine)) = 0;
        }
    }

    cursorPosition.x -= font->header->width;

    if (cursorPosition.x < 0)
    {
        cursorPosition.x = targetFramebuffer->width;
        cursorPosition.y -= font->header->height;

        if (cursorPosition.y < 0) cursorPosition.y = 0;
    }
}


void FramebufferRenderer::Newline()
{
    cursorPosition.x = 0;
    cursorPosition.y += font->header->height;
}

void FramebufferRenderer::Print(const char* str)
{
    if(font == NULL)
    {
        return;
    }

    char* chr = (char*)str;

    while(*chr != 0)
    {
        PutChar(*chr, cursorPosition.x, cursorPosition.y);

        cursorPosition.x += font->header->width;

        if(cursorPosition.x + font->header->width > targetFramebuffer->width)
        {
            cursorPosition.x = 0;
            cursorPosition.y += font->header->height;
        }

        chr++;
    }
}

void FramebufferRenderer::PutChar(char chr, unsigned int xOff, unsigned int yOff)
{
    if(font == NULL)
    {
        return;
    }

    psf2PutChar(xOff, yOff, chr, font, 0xFFFFFFFF, this);
}

void FramebufferRenderer::PutChar(char chr)
{
    PutChar(chr, cursorPosition.x, cursorPosition.y);
    cursorPosition.x += font->header->width;

    if (cursorPosition.x + font->header->width > targetFramebuffer->width)
    {
        cursorPosition.x = 0; 
        cursorPosition.y += font->header->height;
    }
}

FramebufferArea FramebufferRenderer::FrameBufferAreaAt(int x, int y, int width, int height)
{
    FramebufferArea outArea
    {
        .x = x,
        .y = y,
        .width = (int)targetFramebuffer->width,
        .height = (int)targetFramebuffer->height,
        .targetX = 0,
        .targetY = 0,
        .targetWidth = width,
        .targetHeight = height,
        .buffer = NULL,
    };

    if(x >= targetFramebuffer->width || y >= targetFramebuffer->height || width <= 0 || height <= 0)
    {
        return outArea;
    }

    if(x < 0)
    {
        outArea.x = 0;
        outArea.targetX = -x;
        outArea.targetWidth = width + x;
    }

    if(y < 0)
    {
        outArea.y = 0;
        outArea.targetY = -y;
        outArea.targetHeight = height + y;
    }

    if(x + width > targetFramebuffer->width)
    {
        outArea.targetWidth = targetFramebuffer->width - outArea.x;
    }

    if(y + height > targetFramebuffer->height)
    {
        outArea.targetHeight = targetFramebuffer->height - outArea.y;
    }

    if(outArea.targetX < 0 || outArea.targetY < 0 ||
        outArea.targetWidth <= 0 || outArea.targetHeight <= 0)
    {
        return outArea;
    }

    outArea.buffer = (uint32_t *)targetFramebuffer->baseAddress;

    return outArea;
}

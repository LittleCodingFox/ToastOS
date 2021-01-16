#include "FramebufferRenderer.hpp"
#include "debug.hpp"

FramebufferRenderer* GlobalRenderer;

FramebufferRenderer::FramebufferRenderer(Framebuffer* targetFramebuffer, psf2_font_t* font)
{
    TargetFramebuffer = targetFramebuffer;
    this->font = font;
    Colour = 0xffffffff;
    CursorPosition = {0, 0};

    DEBUG_OUT("Received font: %p", font);
}

void FramebufferRenderer::Clear(uint32_t colour)
{
    uint64_t fbBase = (uint64_t)TargetFramebuffer->baseAddress;
    uint64_t bytesPerScanline = TargetFramebuffer->pixelsPerScanLine * 4;
    uint64_t fbHeight = TargetFramebuffer->height;

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
    if (CursorPosition.x == 0)
    {
        CursorPosition.x = TargetFramebuffer->width;
        CursorPosition.y -= font->header->height;

        if (CursorPosition.y < 0) CursorPosition.y = 0;
    }

    unsigned int xOff = CursorPosition.x;
    unsigned int yOff = CursorPosition.y;

    unsigned int* pixPtr = (unsigned int*)TargetFramebuffer->baseAddress;
    for (unsigned long y = yOff; y < yOff + font->header->height; y++)
    {
        for (unsigned long x = xOff - font->header->width; x < xOff; x++)
        {
            *(unsigned int*)(pixPtr + x + (y * TargetFramebuffer->pixelsPerScanLine)) = 0;
        }
    }

    CursorPosition.x -= font->header->width;

    if (CursorPosition.x < 0)
    {
        CursorPosition.x = TargetFramebuffer->width;
        CursorPosition.y -= font->header->height;

        if (CursorPosition.y < 0) CursorPosition.y = 0;
    }
}


void FramebufferRenderer::Newline()
{
    CursorPosition.x = 0;
    CursorPosition.y += font->header->height;
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
        PutChar(*chr, CursorPosition.x, CursorPosition.y);

        CursorPosition.x+=font->header->width;

        if(CursorPosition.x + font->header->width > TargetFramebuffer->width)
        {
            CursorPosition.x = 0;
            CursorPosition.y += font->header->height;
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

    psf2PutChar(xOff, yOff, chr, font, this);
}

uint32_t *FramebufferRenderer::FrameBufferPtrAt(int x, int y)
{
    if(x < 0 || y < 0 || x >= TargetFramebuffer->width || y >= TargetFramebuffer->height)
    {
        return NULL;
    }

    return ((uint32_t *)TargetFramebuffer->baseAddress) + x + (y * TargetFramebuffer->width);
}

void FramebufferRenderer::PutChar(char chr)
{
    PutChar(chr, CursorPosition.x, CursorPosition.y);
    CursorPosition.x += font->header->width;
    
    if (CursorPosition.x + font->header->width > TargetFramebuffer->width)
    {
        CursorPosition.x = 0; 
        CursorPosition.y += 16;
    }
}
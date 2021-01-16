#include "FramebufferRenderer.hpp"
#include "debug.hpp"

FramebufferRenderer* GlobalRenderer;

FramebufferRenderer::FramebufferRenderer(Framebuffer* targetFramebuffer, psf2_font_t* font)
{
    this->targetFramebuffer = targetFramebuffer;
    this->font = font;

    Colour = 0xffffffff;
    CursorPosition = {0, 0};

    DEBUG_OUT("Received font: %p", font);
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
    if (CursorPosition.x == 0)
    {
        CursorPosition.x = targetFramebuffer->width;
        CursorPosition.y -= font->header->height;

        if (CursorPosition.y < 0) CursorPosition.y = 0;
    }

    unsigned int xOff = CursorPosition.x;
    unsigned int yOff = CursorPosition.y;

    unsigned int* pixPtr = (unsigned int*)targetFramebuffer->baseAddress;
    for (unsigned long y = yOff; y < yOff + font->header->height; y++)
    {
        for (unsigned long x = xOff - font->header->width; x < xOff; x++)
        {
            *(unsigned int*)(pixPtr + x + (y * targetFramebuffer->pixelsPerScanLine)) = 0;
        }
    }

    CursorPosition.x -= font->header->width;

    if (CursorPosition.x < 0)
    {
        CursorPosition.x = targetFramebuffer->width;
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

        if(CursorPosition.x + font->header->width > targetFramebuffer->width)
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

void FramebufferRenderer::PutChar(char chr)
{
    PutChar(chr, CursorPosition.x, CursorPosition.y);
    CursorPosition.x += font->header->width;

    if (CursorPosition.x + font->header->width > targetFramebuffer->width)
    {
        CursorPosition.x = 0; 
        CursorPosition.y += 16;
    }
}

FramebufferArea FramebufferRenderer::FrameBufferAreaAt(int x, int y, int width, int height)
{
    FramebufferArea outArea = { 0 };
    outArea.x = x;
    outArea.y = y;
    outArea.width = targetFramebuffer->width;
    outArea.height = targetFramebuffer->height;
    outArea.targetX = 0;
    outArea.targetY = 0;
    outArea.targetWidth = width;
    outArea.targetHeight = height;
    outArea.buffer = NULL;

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

#include "FramebufferRenderer.hpp"
#include "debug.hpp"
#include "paging/PageFrameAllocator.hpp"
#include "timer/Timer.hpp"
#include <stdlib.h>
#include <string.h>

FramebufferRenderer* globalRenderer;

void RefreshFramebuffer(InterruptStack *stack)
{
    globalRenderer->SwapBuffers();
}

FramebufferRenderer::FramebufferRenderer(Framebuffer* targetFramebuffer, psf2_font_t* font) : lockedCount(0),
    doubleBuffer(NULL), initialized(false), doubleBufferSize(0), graphicsType(GRAPHICS_TYPE_CONSOLE), backgroundColor(0)
{
    this->targetFramebuffer = targetFramebuffer;
    this->font = font;

    framebufferPixelCount = targetFramebuffer->width * targetFramebuffer->height;
}

void FramebufferRenderer::Initialize()
{
    if(initialized)
    {
        return;
    }

    initialized = true;

    doubleBufferSize = sizeof(uint32_t) * targetFramebuffer->width * targetFramebuffer->height;

    doubleBuffer = (uint32_t *)malloc(doubleBufferSize);

    Clear(0);
}

void FramebufferRenderer::SetGraphicsBuffer(const void *buffer)
{
    if(graphicsType != GRAPHICS_TYPE_GUI)
    {
        return;
    }

    memcpy(doubleBuffer, buffer, doubleBufferSize);
}

void FramebufferRenderer::SetGraphicsType(int graphicsType)
{
    switch(graphicsType)
    {
        case GRAPHICS_TYPE_CONSOLE:

        this->graphicsType = GRAPHICS_TYPE_CONSOLE;

        //TODO
        break;

        case GRAPHICS_TYPE_GUI:

        this->graphicsType = GRAPHICS_TYPE_GUI;

        Clear(0);

        break;        
    }
}

void FramebufferRenderer::Lock()
{
    lockedCount++;
}

void FramebufferRenderer::Unlock()
{
    if(lockedCount == 0)
    {
        DEBUG_OUT("%s", "Unexpectedly unlocking framebuffer renderer while it's already unlocked!");

        return;
    }

    lockedCount--;
}

void FramebufferRenderer::SwapBuffers()
{
    if(doubleBuffer != NULL && lockedCount == 0)
    {
        memcpy(targetFramebuffer->baseAddress, doubleBuffer, doubleBufferSize);
    }
}

void FramebufferRenderer::Clear(uint32_t colour)
{
    backgroundColor = colour;

    for(uint32_t i = 0; i < framebufferPixelCount; i++)
    {
        doubleBuffer[i] = colour;
    }
}

void FramebufferRenderer::PutChar(char chr, unsigned int xOff, unsigned int yOff, uint32_t color)
{
    if(font == NULL || graphicsType != GRAPHICS_TYPE_CONSOLE)
    {
        return;
    }

    psf2PutChar(xOff, yOff, chr, font, color, this);
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

    outArea.buffer = doubleBuffer != NULL ? doubleBuffer : (uint32_t *)targetFramebuffer->baseAddress;

    return outArea;
}

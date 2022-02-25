#pragma once

#include "kernel.h"

class FramebufferRenderer;

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t headerSize;
    uint32_t flags;
    uint32_t numGlyph;
    uint32_t bytesPerGlyph;
    uint32_t height;
    uint32_t width;
} PACKED psf2_header_t;

typedef struct
{
    psf2_header_t* header;
    void* glyph_buffer;
} psf2_font_t;

typedef struct {
	uint32_t width;
	uint32_t height;
} psf2_size_t;

psf2_font_t *psf2Load(void *buffer);
void psf2PutChar(int x, int y, char c, psf2_font_t *font, uint32_t color, FramebufferRenderer *renderer);
void psf2RenderText(int x, int y, const char *text, psf2_font_t *font, uint32_t color, FramebufferRenderer *renderer);
psf2_size_t psf2MeasureText(const char *text, psf2_font_t *font);

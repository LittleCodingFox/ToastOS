#include <stdint.h>
#include <framebuffer/FramebufferRenderer.hpp>
#include <stdlib.h>
#include <string.h>
#include "debug.hpp"
#include "psf2.hpp"

#define PSF2_MAGIC 0x864ab572

psf2_font_t *psf2Load(void *buffer)
{
	psf2_header_t *header = (psf2_header_t *)buffer;

	if(header->magic != PSF2_MAGIC)
	{
		DEBUG_OUT("%s", "PSF2 Load invalid magic");

		return NULL;
	}

	uint64_t glyphBufferSize = header->numGlyph * header->bytesPerGlyph;

	void *glyphBuffer = malloc(glyphBufferSize);

	memcpy(glyphBuffer, (uint8_t *)buffer + header->headerSize, glyphBufferSize);

	psf2_font_t *outValue = (psf2_font_t *)malloc(sizeof(psf2_font_t));

	outValue->glyph_buffer = glyphBuffer;
	outValue->header = header;

	return outValue;
}

psf2_size_t psf2MeasureText(const char *text, psf2_font_t *font)
{
	psf2_size_t outValue;
	outValue.width = 0;
	outValue.height = 0;
	
	if(font == NULL)
	{
		return outValue;
	}

	int counter = 0;
	int lineCounter = 1;
	int maxWidth = 0;

	while(*text)
	{
		if(*text == '\n')
		{
			maxWidth = maxWidth < counter ? counter : maxWidth;
			counter = 0;
			lineCounter++;
		}
		else
		{
			counter++;
		}
	}

	if(counter > 0)
	{
		maxWidth = maxWidth < counter ? counter : maxWidth;
	}

	outValue.width = maxWidth * (font->header->width - 1);
	outValue.height = lineCounter * font->header->height;

	return outValue;
}

void psf2PutChar(int x, int y, char c, psf2_font_t *font, uint32_t color, FramebufferRenderer *renderer)
{
	if(font == NULL)
	{
		return;
	}

	FramebufferArea area = renderer->FrameBufferAreaAt(x, y, font->header->width, font->header->height);

	if(!area.IsValid())
	{
		return;
	}

	int bytesPerLine = (font->header->width + 7) / 8;
	unsigned char *glyph = (unsigned char*)font->glyph_buffer + (c > 0 && c < font->header->numGlyph ? c : 0) * font->header->bytesPerGlyph +
		area.targetY * bytesPerLine;

	for(int ty = 0, yIndex = area.y * area.width; ty < area.targetHeight; ty++, yIndex += area.width)
	{
		int mask = 1 << (font->header->width - 1);
		mask >>= area.targetX;

		for(int tx = 0; tx < area.targetWidth; tx++)
		{
			area.buffer[yIndex + area.x + tx] = ((int)*glyph) & (mask) ? color : renderer->backgroundColor;
			mask >>= 1;
		}

		glyph+=bytesPerLine;
	}
}

void psf2RenderText(int x, int y, const char *text, psf2_font_t *font, uint32_t color, FramebufferRenderer *renderer)
{
	if(font == NULL)
	{
		return;
	}

	int characterx = 0;

	while(*text)
	{
		if(*text == '\n')
		{
			y += font->header->height;

			text++;
			characterx = 0;

			continue;
		}

		int offset = characterx * (font->header->width + 1);

		psf2PutChar(x + offset, y, *text, font, color, renderer);

		text++;
		characterx++;
	}
}

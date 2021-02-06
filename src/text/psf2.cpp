#include <stdint.h>
#include <framebuffer/FramebufferRenderer.hpp>
#include "debug.hpp"
#include "psf2.hpp"

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
		int line = 0;
		int mask = 1 << (font->header->width - 1);
		mask >>= area.targetX;

		for(int tx = 0; tx < area.targetWidth; tx++)
		{
			area.buffer[yIndex + area.x + tx] = ((int)*glyph) & (mask) ? color : 0;
			mask >>= 1;
		}

		glyph+=bytesPerLine;
	}
}

void psf2RenderText(int x, int y, const char *text, uint32_t color, psf2_font_t *font, FramebufferRenderer *renderer)
{
	if(font == NULL)
	{
		return;
	}

	int characterx = 0;

	while(*text)
	{
		int offset = characterx * (font->header->width + 1);

		psf2PutChar(x + offset, y, *text, font, color, renderer);

		text++;
		characterx++;
	}
}

#include <stdint.h>
#include <framebuffer/Framebuffer.hpp>
#include <framebuffer/BasicRenderer.hpp>
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

void psf2PutChar(int x, int y, char c, psf2_font_t *font, BasicRenderer *renderer)
{
	if(font == NULL)
	{
		return;
	}

	int bytesPerLine = (font->header->width + 7) / 8;
	unsigned char *glyph = (unsigned char*)font->glyph_buffer + (c > 0 && c < font->header->numGlyph ? c : 0) * font->header->bytesPerGlyph;

	for(int ty=0; ty<font->header->height; ty++)
	{
		int line = 0;
		int mask = 1 << (font->header->width - 1);

		uint32_t *pixels = renderer->FrameBufferPtrAt(x, y + ty);

		for(int tx = 0; tx < font->header->width; tx++)
		{
			pixels[line] = ((int)*glyph) & (mask) ? 0xFFFFFFFF : 0;

			mask >>= 1;
			line++;
		}

		pixels[line] = 0;
		glyph+=bytesPerLine;
	}
}

void psf2RenderText(int x, int y, const char *text, psf2_font_t *font, BasicRenderer *renderer)
{
	if(font == NULL)
	{
		return;
	}

	int characterx = 0;

	while(*text)
	{
		int offset = characterx * (font->header->width + 1);

		psf2PutChar(x + offset, y, *text, font, renderer);

		text++;
		characterx++;
	}
}

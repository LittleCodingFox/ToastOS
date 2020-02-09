#include <stdint.h>
#include <bootboot.h>
#include <framebuffer/framebuffer.h>
#include "psf2.h"
extern BOOTBOOT bootboot;           // see bootboot.h
extern unsigned char *environment;  // configuration, UTF-8 text key=value pairs
extern uint8_t fb;                  // linear framebuffer mapped
extern volatile unsigned char _binary_font_psf_start;

psf2_t *kernel_default_font = NULL;

void kernel_init_text()
{
	kernel_default_font = (psf2_t *)&_binary_font_psf_start;
}

psf2_size_t kernel_measure_text(const char *text, psf2_t *font)
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

	outValue.width = maxWidth * (font->width - 1);
	outValue.height = lineCounter * font->height;

	return outValue;
}

void kernel_render_text(int x, int y, const char *text, psf2_t *font)
{
	if(font == NULL)
	{
		return;
	}

	unsigned char *fontData = (unsigned char*)font;
	int bytesPerLine = (font->width + 7) / 8;
	int characterx = 0;

	while(*text)
	{
		unsigned char *glyph = fontData + font->headersize +
            (*text > 0 && *text < font->numglyph ? *text : 0) * font->bytesperglyph;
		int offset = (characterx * (font->width + 1));

	        for(int ty=0; ty<font->height; ty++)
		{
			int line = offset;
			int mask = 1 << (font->width - 1);

			for(int tx = 0; tx < font->width; tx++) {
				framebuffer_put_pixel(x + line, y + ty, ((int)*glyph) & (mask) ? 0xFFFFFF : 0);
		                mask >>= 1;
				line++;
    			}

			framebuffer_put_pixel(x + line, y + ty, 0);
		        glyph+=bytesPerLine;
	        }

        	text++;
		characterx++;
	}
}


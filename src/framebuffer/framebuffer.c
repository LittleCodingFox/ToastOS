#include <stdint.h>
#include <bootboot.h>
#include "framebuffer.h"
extern BOOTBOOT bootboot;
extern uint8_t fb; //Framebuffer ptr

////// INTERNAL USAGE
int internal_framebuffer_width()
{
	return bootboot.fb_width;
}

int internal_framebuffer_height()
{
	return bootboot.fb_height;
}

int internal_framebuffer_scanline()
{
	return bootboot.fb_scanline;
}

void internal_framebuffer_put_pixel(int x, int y, uint32_t color)
{
	*((uint32_t*)(&fb + y * internal_framebuffer_scanline() + x * sizeof(uint32_t))) = color;
}

void internal_framebuffer_put_pixels(int x, int y, int blockWidth, int blockHeight, uint32_t color)
{
	int width = internal_framebuffer_width();
	int height = internal_framebuffer_height();
	int scanline = internal_framebuffer_scanline();

	if(x < 0 || x + blockWidth > width ||
		y < 0 || y + blockHeight > height)
	{
		return;
	}

	for(int ty = 0, yIndex = y * scanline; ty < blockHeight; ty++, yIndex += scanline)
	{
		for(int tx = 0, xIndex = x * sizeof(uint32_t); tx < blockWidth; tx++, xIndex += sizeof(uint32_t))
		{
			*((uint32_t*)(&fb + yIndex + xIndex)) = color;
		}
	}
}

void internal_framebuffer_clear(uint32_t color)
{
	internal_framebuffer_put_pixels(0, 0, internal_framebuffer_width(), internal_framebuffer_height(), color);
}

///// PUBLIC INTERFACE
void framebuffer_init()
{
	framebuffer_clear(0x00000000);
}

int framebuffer_width()
{
	return internal_framebuffer_width();
}

int framebuffer_height()
{
	return internal_framebuffer_height();
}

void framebuffer_clear(uint32_t color)
{
	internal_framebuffer_clear(color);
}

void framebuffer_put_pixel(int x, int y, uint32_t color)
{
	internal_framebuffer_put_pixel(x, y, color);
}

void framebuffer_put_pixels(int x, int y, int blockWidth, int blockHeight, uint32_t color)
{
	internal_framebuffer_put_pixels(x, y, blockWidth, blockHeight, color);
}

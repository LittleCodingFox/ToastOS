#include <stdint.h>
#include <bootboot.h>
#include "framebuffer.h"
extern BOOTBOOT bootboot;
extern uint8_t fb; //Framebuffer ptr

////// INTERNAL USAGE
int internalFramebufferWidth()
{
	return bootboot.fb_width;
}

int internalFramebufferHeight()
{
	return bootboot.fb_height;
}

int internalFramebufferScanline()
{
	return bootboot.fb_scanline;
}

void internalFramebufferPutPixel(int x, int y, uint32_t color)
{
	*((uint32_t*)(&fb + y * internalFramebufferScanline() + x * sizeof(uint32_t))) = color;
}

void internalFramebufferPutPixels(int x, int y, int blockWidth, int blockHeight, uint32_t color)
{
	int width = internalFramebufferWidth();
	int height = internalFramebufferHeight();
	int scanline = internalFramebufferScanline();

	if(x < 0 || x + blockWidth > width ||
		y < 0 || y + blockHeight > height) {

		return;
	}

	for(int ty = 0, yIndex = y * scanline; ty < blockHeight; ty++, yIndex += scanline) {

		for(int tx = 0, xIndex = x * sizeof(uint32_t); tx < blockWidth; tx++, xIndex += sizeof(uint32_t)) {

			*((uint32_t*)(&fb + yIndex + xIndex)) = color;
		}
	}
}

void internalFramebufferClear(uint32_t color)
{
	internalFramebufferPutPixels(0, 0, internalFramebufferWidth(), internalFramebufferHeight(), color);
}

///// PUBLIC INTERFACE
void framebufferInit()
{
	framebufferClear(0);
}

int framebufferWidth()
{
	return internalFramebufferWidth();
}

int framebufferHeight()
{
	return internalFramebufferHeight();
}

void framebufferClear(uint32_t color)
{
	internalFramebufferClear(color);
}

void framebufferPutPixel(int x, int y, uint32_t color)
{
	internalFramebufferPutPixel(x, y, color);
}

void framebufferPutPixels(int x, int y, int blockWidth, int blockHeight, uint32_t color)
{
	internalFramebufferPutPixels(x, y, blockWidth, blockHeight, color);
}

#pragma once
#include <stddef.h>

struct Framebuffer
{
	void* baseAddress;
	size_t bufferSize;
	unsigned int width;
	unsigned int height;
	unsigned int pixelsPerScanLine;
};

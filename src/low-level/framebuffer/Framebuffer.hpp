#pragma once
#include <stdint.h>
#include <stddef.h>

struct Framebuffer
{
	void* baseAddress;
	size_t bufferSize;
	int32_t width;
	int32_t height;
	int32_t pixelsPerScanLine;
};

#pragma once

#include <stdint.h>

void framebufferInit();
int framebufferWidth();
int framebufferHeight();
void framebufferClear(uint32_t color);
void framebufferPutPixel(int x, int y, uint32_t color);
void framebufferPutPixels(int x, int y, int blockWidth, int blockHeight, uint32_t color);

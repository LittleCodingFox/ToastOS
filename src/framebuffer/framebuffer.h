#pragma once

#include <stdint.h>

void framebuffer_init();
int framebuffer_width();
int framebuffer_height();
void framebuffer_clear(uint32_t color);
void framebuffer_put_pixel(int x, int y, uint32_t color);
void framebuffer_put_pixels(int x, int y, int blockWidth, int blockHeight, uint32_t color);

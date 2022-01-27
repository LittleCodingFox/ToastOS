#pragma once

#if __cplusplus
extern "C" {
#endif

enum 
{
    GRAPHICS_TYPE_CONSOLE,
    GRAPHICS_TYPE_GUI,
};

void ToastSetGraphicsType(int type);

void ToastGetGraphicsSize(int *width, int *height, int *bpp);

void ToastSetGraphicsBuffer(const void *buffer);

#if __cplusplus
}
#endif

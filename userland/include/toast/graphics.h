#pragma once

#if __cplusplus
extern "C" {
#endif

enum 
{
    TOAST_GRAPHICS_TYPE_CONSOLE,
    TOAST_GRAPHICS_TYPE_GUI,
};

void ToastSetGraphicsType(int type);

void ToastGetGraphicsSize(int *width, int *height, int *bpp);

void ToastSetGraphicsBuffer(const void *buffer);

#if __cplusplus
}
#endif

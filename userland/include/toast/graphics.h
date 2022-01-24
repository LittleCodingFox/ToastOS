#pragma once

#if __cplusplus
extern "C" {
#endif

enum 
{
    GRAPHICS_TYPE_CONSOLE,
    GRAPHICS_TYPE_UI,
};

void toastSetGraphicsType(int type);

void toastGetGraphicsSize(int *width, int *height, int *bpp);

void toastSetGraphicsBuffer(const void *buffer);

#if __cplusplus
}
#endif

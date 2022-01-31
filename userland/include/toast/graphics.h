#pragma once

#if __cplusplus
extern "C" {
#endif

enum 
{
    TOAST_GRAPHICS_TYPE_CONSOLE,
    TOAST_GRAPHICS_TYPE_GUI,
};

/**
 * @brief Sets the active graphics type
 * 
 * @param type One of TOAST_GRAPHICS_*. If console, will draw the active console process, if any. If GUI, will be in a graphical UI mode.
 */
void ToastSetGraphicsType(int type);

/**
 * @brief Gets the graphics size of the screen. The value changes depending on whether you're using a centered graphics context.
 * 
 * @param width a pointer to receive the width
 * @param height a pointer to receive the height
 * @param bpp a pointer to receive the bpp. It should always be 32.
 */
void ToastGetGraphicsSize(int *width, int *height, int *bpp);

/**
 * @brief Updates the graphics buffer with a new buffer
 * 
 * @param buffer the new buffer, which should be width x height x sizeof(uint32_t) sized.
 *  If you created a centered graphics context, it should be that way instead.
 */
void ToastSetGraphicsBuffer(const void *buffer);

/**
 * @brief Creates a graphics context using mesa that will be centered in the screen
 * 
 * @param width the width of the screen
 * @param height the height of the screen
 * @return 1 on success, 0 on failure
 */
int ToastCreateCenteredGraphicsContext(int width, int height);

#if __cplusplus
}
#endif

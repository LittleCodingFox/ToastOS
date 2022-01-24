#include <string.h>
#include <toast/graphics.h>
#include <stdint.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    toastSetGraphicsType(GRAPHICS_TYPE_UI);

    int width, height, bpp;

    toastGetGraphicsSize(&width, &height, &bpp);

    int bufferByteSize = sizeof(uint32_t) * width * height;

    uint32_t *buffer = (uint32_t *)malloc(bufferByteSize);

    memset(buffer, 128, bufferByteSize);

    toastSetGraphicsBuffer(buffer);

    for(;;);

    return 0;
}

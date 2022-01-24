#include <string.h>
#include <toast/graphics.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <GLES/gl.h>
#include <GL/osmesa.h>

int main(int argc, char **argv)
{
    printf("Setting graphics type to UI\n");
    toastSetGraphicsType(GRAPHICS_TYPE_UI);

    int width, height, bpp;

    toastGetGraphicsSize(&width, &height, &bpp);

    printf("Graphics Size is: %dx%d (%d)\n", width, height, bpp);

    int bufferByteSize = sizeof(uint32_t) * width * height;

    uint32_t *buffer = (uint32_t *)malloc(bufferByteSize);

    printf("Making current\n");

    OSMesaContext GLContext = OSMesaCreateContext(OSMESA_RGBA, NULL);

    if (!OSMesaMakeCurrent(GLContext, buffer, GL_UNSIGNED_BYTE, width, height))
    {
        printf("OSMesa failed to make current\n");

		return 1;
    }

	OSMesaPixelStore(OSMESA_Y_UP, 0);

    printf("Set viewport\n");

    glViewport(0, 0, width, height);

    printf("Entering main loop!\n");

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    int frames = 0;
    time_t t = time(NULL);

    for(;;)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glFinish();

        frames++;

        time_t current = time(NULL);

        auto diff = difftime(current, t);

        if(diff >= 1.0)
        {
            printf("FPS: %d\n", frames);

            frames = 0;

            t = current;
        }

        toastSetGraphicsBuffer(buffer);
    }

    return 0;
}

#include <string.h>
#include <toast/graphics.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <GLES/gl.h>
#include <GL/osmesa.h>

uint32_t GetTime()
{
  struct timespec currentTime;

  clock_gettime(CLOCK_MONOTONIC, &currentTime);

  return currentTime.tv_sec * 1000 + currentTime.tv_nsec / 1000000;
}

int main(int argc, char **argv)
{
    printf("Setting graphics type to GUI\n");

    ToastSetGraphicsType(GRAPHICS_TYPE_GUI);

    int width, height, bpp;

    ToastGetGraphicsSize(&width, &height, &bpp);

    printf("Graphics Size is: %dx%d (%d)\n", width, height, bpp);

    int bufferByteSize = sizeof(uint32_t) * width * height;

    uint32_t *buffer = (uint32_t *)malloc(bufferByteSize);

    memset(buffer, 128, bufferByteSize);

    ToastSetGraphicsBuffer(buffer);

    printf("Making current\n");

    OSMesaContext GLContext = OSMesaCreateContext(OSMESA_BGRA, NULL);

    if (!OSMesaMakeCurrent(GLContext, buffer, GL_UNSIGNED_BYTE, width, height))
    {
        printf("OSMesa failed to make current\n");

		return 1;
    }

	OSMesaPixelStore(OSMESA_Y_UP, 0);

    printf("Set viewport\n");

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    printf("Entering main loop!\n");

    glClearColor(0.0f, 1.0f, 1.0f, 0.0f);

    int frames = 0;
    uint32_t start = GetTime();
    uint32_t t = GetTime();

    float angle = 0.0f;

    for(;;)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glColor3f(1, 1, 1);

        glPushMatrix();
        glTranslatef(width / 2, height / 2, 0);
        glRotatef(angle, 0, 0, 1);

        glBegin(GL_QUADS);
        glVertex2f(-50, -50);
        glVertex2f(-50, 50);
        glVertex2f(50, 50);
        glVertex2f(50, -50);
        glEnd();

        glPopMatrix();

        glFinish();

        frames++;

        uint32_t current = GetTime();

        uint32_t difference = current - t;

        float delta = (current - t) / 1000.0f;

        t = current;

        angle += delta * 30;

        if(current - start >= 1000)
        {
            printf("FPS: %d\n", frames);

            frames = 0;

            start = current;
        }

        ToastSetGraphicsBuffer(buffer);
    }

    return 0;
}

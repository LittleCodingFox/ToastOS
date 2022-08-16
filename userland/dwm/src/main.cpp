#include <string.h>
#include <toast/graphics.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <GLES/gl.h>
#include <GL/osmesa.h>

#include "Screen.hpp"

void DrawLogo(float delta);
void DrawMain(float delta);

int screenWidth = 0;
int screenHeight = 0;

extern Screen logoScreen;

extern Screen mainScreen;

Screen *currentScreen = &logoScreen;

uint32_t GetTime()
{
  struct timespec currentTime;

  clock_gettime(CLOCK_MONOTONIC, &currentTime);

  return currentTime.tv_sec * 1000 + currentTime.tv_nsec / 1000000;
}

int main(int argc, char **argv)
{
    printf("Setting graphics type to GUI\n");

    ToastSetGraphicsType(TOAST_GRAPHICS_TYPE_GUI);

    int bpp;

    ToastGetGraphicsSize(&screenWidth, &screenHeight, &bpp);

    printf("Graphics Size is: %dx%d (%d)\n", screenWidth, screenHeight, bpp);

    int bufferByteSize = sizeof(uint32_t) * screenWidth * screenHeight;

    uint32_t *buffer = (uint32_t *)malloc(bufferByteSize);

    memset(buffer, 128, bufferByteSize);

    ToastSetGraphicsBuffer(buffer);

    printf("Making current\n");

    OSMesaContext GLContext = OSMesaCreateContext(OSMESA_BGRA, NULL);

    if (!OSMesaMakeCurrent(GLContext, buffer, GL_UNSIGNED_BYTE, screenWidth, screenHeight))
    {
        printf("OSMesa failed to make current\n");

		return 1;
    }

	OSMesaPixelStore(OSMESA_Y_UP, 0);

    printf("Set viewport\n");

    glViewport(0, 0, screenWidth, screenHeight);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, screenWidth, screenHeight, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    printf("Entering main loop!\n");

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    int frames = 0;
    uint32_t t = GetTime();

    for(;;)
    {
        uint32_t current = GetTime();

        float delta = (current - t) / 1000.0f;

        t = current;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if(currentScreen != NULL && currentScreen->draw != NULL)
        {
            currentScreen->draw(delta);
        }

        glFinish();

        frames++;

        ToastSetGraphicsBuffer(buffer);
    }

    return 0;
}

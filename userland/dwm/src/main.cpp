#include <string.h>
#include <toast/graphics.h>
#include <toast/input.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <GLES/gl.h>
#include <GL/osmesa.h>

#include "App.hpp"
#include "Screen.hpp"

void DrawLogo(float delta);
void DrawMain(float delta);

int socketFD;

int screenWidth = 0;
int screenHeight = 0;

int mouseX = 0;
int mouseY = 0;
int mouseButtons = 0;
bool mouseVisible = true;

int frames = 0;
int fps = 0;
float frameTimer = 0;

extern Screen logoScreen;

extern Screen mainScreen;

Screen *currentScreen = &mainScreen;

uint32_t GetTime()
{
  struct timespec currentTime;

  clock_gettime(CLOCK_MONOTONIC, &currentTime);

  return currentTime.tv_sec * 1000 + currentTime.tv_nsec / 1000000;
}

int main(int argc, char **argv)
{
    printf("Creating socket\n");

    socketFD = socket(PF_UNIX, SOCK_DGRAM, 0);

    if(socketFD < 0)
    {
        printf("Failed to create socket\n");

        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/tmp/dwm/socket");

    if(bind(socketFD, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("Failed to bind socket\n");

        return 1;
    }

    EnumerateApps();

    for(auto &app : *apps.get())
    {
        printf("Found App: %s (icon: %s, exe: %s)\n", app.name.data(), app.icon.data(), app.exe.data());
    }

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

    printf("OpenGL Info:\nVendor: %s\nRenderer: %s\n Version: %s",
        glGetString(GL_VENDOR),
        glGetString(GL_RENDERER),
        glGetString(GL_VERSION));

    glShadeModel(GL_FLAT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_DITHER);

    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    printf("Set viewport\n");

    glViewport(0, 0, screenWidth, screenHeight);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, screenWidth, screenHeight, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    printf("Entering main loop!\n");

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    uint32_t t = GetTime();

    InputEvent inputEvent;

    char buff[1024];

    for(;;)
    {
        uint32_t current = GetTime();

        float delta = (current - t) / 1000.0f;

        t = current;

        frameTimer += delta;
        frames++;

        if(frameTimer > 1)
        {
            frameTimer = 0;
            fps = frames;
            frames = 0;

            printf("FPS: %i\n", fps);
        }

        if(currentScreen != NULL && currentScreen->draw != NULL)
        {
            currentScreen->draw(delta);
        }

        glFinish();

        while(ToastInputPollEvent(&inputEvent))
        {
            switch(inputEvent.type)
            {
                case TOAST_INPUT_EVENT_MOUSEMOVE:

                    mouseX = inputEvent.mouseEvent.x;
                    mouseY = inputEvent.mouseEvent.y;
                    mouseButtons = inputEvent.mouseEvent.buttons;

                    break;

                case TOAST_INPUT_EVENT_MOUSEDOWN:

                    mouseButtons |= inputEvent.mouseEvent.buttons;

                    break;

                case TOAST_INPUT_EVENT_MOUSEUP:

                    mouseButtons &= ~inputEvent.mouseEvent.buttons;

                    break;
            }
        }

        ToastSetGraphicsBuffer(buffer);
    }

    return 0;
}

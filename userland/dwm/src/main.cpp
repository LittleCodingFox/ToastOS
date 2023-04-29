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

#include "Screen.hpp"

void DrawLogo(float delta);
void DrawMain(float delta);

int socketFD;

int screenWidth = 0;
int screenHeight = 0;

int mouseX = 0;
int mouseY = 0;
int mouseButtons = 0;

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

    printf("Creating socket\n");

    socketFD = socket(PF_UNIX, SOCK_DGRAM, 0);

    if(socketFD < 0)
    {
        printf("Failed to create socket\n");

        OSMesaMakeCurrent(NULL, NULL, GL_UNSIGNED_BYTE, 0, 0);

        OSMesaDestroyContext(GLContext);

        ToastSetGraphicsType(TOAST_GRAPHICS_TYPE_CONSOLE);

        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/tmp/dwm/socket");

    if(bind(socketFD, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("Failed to bind socket\n");

        OSMesaMakeCurrent(NULL, NULL, GL_UNSIGNED_BYTE, 0, 0);

        OSMesaDestroyContext(GLContext);

        ToastSetGraphicsType(TOAST_GRAPHICS_TYPE_CONSOLE);

        return 1;
    }

    printf("Entering main loop!\n");

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    uint32_t t = GetTime();

    InputEvent inputEvent;

    char buff[1024];

    for(;;)
    {
        struct sockaddr_un from;
        socklen_t fromLength = sizeof(from);
        int length;

        while((length = recvfrom(socketFD, buff, sizeof(buff), 0, (struct sockaddr *)&from, &fromLength)) > 0)
        {
            printf("received: %s\n", buff);

            strcpy(buff, "OK");

            int result = sendto(socketFD, buff, strlen(buff) + 1, 0, (struct sockaddr *)&from, fromLength);

            if(result < 0)
            {
                printf("Error: sendto\n");

                return 1;
            }
        }

        uint32_t current = GetTime();

        float delta = (current - t) / 1000.0f;

        t = current;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if(currentScreen != NULL && currentScreen->draw != NULL)
        {
            currentScreen->draw(delta);
        }

        glFinish();

        if(ToastInputPollEvent(&inputEvent))
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

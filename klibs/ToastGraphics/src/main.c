#include <toast/syscall.h>
#include <toast/graphics.h>
#include <stdlib.h>
#include <stdbool.h>
#include <GL/gl.h>
#include <GL/osmesa.h>
#include <string.h>

bool usingCenteredGraphicsContext = false;
int centeredGraphicsWidth = 0;
int centeredGraphicsHeight = 0;
int graphicsWidth = 0;
int graphicsHeight = 0;
uint32_t *mesaBuffer = NULL;
float scaleFactor = 1.0f;
float offsetX = 0;
float offsetY = 0;

void ToastSetGraphicsType(int type)
{
    syscall(SYSCALL_SETGRAPHICSTYPE, type);
}

void ToastGetGraphicsSize(int *width, int *height, int *bpp)
{
    if(usingCenteredGraphicsContext)
    {
        *width = centeredGraphicsWidth;
        *height = centeredGraphicsHeight;
        *bpp = 32;

        return;
    }

    syscall(SYSCALL_GETGRAPHICSSIZE, width, height, bpp);
}

void ToastSetGraphicsBuffer(const void *buffer)
{
    if(usingCenteredGraphicsContext)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glColor3f(1, 1, 1);

        float width = centeredGraphicsWidth * scaleFactor;
        float height =  centeredGraphicsHeight * scaleFactor;

        glBegin(GL_QUADS);
        glVertex2f(offsetX, offsetY);
        glVertex2f(offsetX, offsetY + height);
        glVertex2f(offsetX + width, offsetY + height);
        glVertex2f(offsetX + width, offsetY);
        glEnd();

        glFinish();

        syscall(SYSCALL_SETGRAPHICSBUFFER, mesaBuffer);

        return;
    }

    syscall(SYSCALL_SETGRAPHICSBUFFER, buffer);
}

int ToastCreateCenteredGraphicsContext(int width, int height)
{
    if(usingCenteredGraphicsContext)
    {
        return 0;
    }

    ToastSetGraphicsType(TOAST_GRAPHICS_TYPE_GUI);

    int bpp;

    ToastGetGraphicsSize(&graphicsWidth, &graphicsHeight, &bpp);

    int bufferByteSize = sizeof(uint32_t) * width * height;

    uint32_t *mesaBuffer = (uint32_t *)malloc(bufferByteSize);

    memset(mesaBuffer, 128, bufferByteSize);

    ToastSetGraphicsBuffer(mesaBuffer);

    OSMesaContext GLContext = OSMesaCreateContext(OSMESA_BGRA, NULL);

    if (!OSMesaMakeCurrent(GLContext, mesaBuffer, GL_UNSIGNED_BYTE, width, height))
    {
        ToastSetGraphicsType(TOAST_GRAPHICS_TYPE_CONSOLE);

        free(mesaBuffer);

		return 0;
    }

	OSMesaPixelStore(OSMESA_Y_UP, 0);

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.5f, 0.5f, 0.5f, 0.0f);

    if(width > height)
    {
        if(width > graphicsWidth)
        {
            scaleFactor = graphicsWidth / (float)width;
        }
        else
        {
            scaleFactor = width / (float)graphicsWidth;
        }
    }
    else
    {
        if(height > graphicsHeight)
        {
            scaleFactor = graphicsHeight / (float)height;
        }
        else
        {
            scaleFactor = height / (float)graphicsHeight;
        }
    }

    if(graphicsWidth > width)
    {
        offsetX = (graphicsWidth - width) / 2;
    }

    if(graphicsHeight > height)
    {
        offsetY = (graphicsHeight - height) / 2;
    }

    centeredGraphicsWidth = width;
    centeredGraphicsHeight = height;
    usingCenteredGraphicsContext = true;

    return 1;
}

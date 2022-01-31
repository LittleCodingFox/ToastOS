#include <toast/syscall.h>
#include <toast/graphics.h>
#include <stdlib.h>
#include <stdbool.h>
#include <GL/gl.h>
#include <GL/osmesa.h>
#include <string.h>
#include <stdio.h>

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

        glPushMatrix();
        glTranslatef(graphicsWidth / 2, graphicsHeight / 2, 0);

        float width = graphicsWidth * scaleFactor;
        float height = graphicsHeight * scaleFactor;

        glBegin(GL_QUADS);
        glVertex2f(offsetX, offsetY);
        glVertex2f(offsetX, offsetY + height);
        glVertex2f(offsetX + width, offsetY + height);
        glVertex2f(offsetX + width, offsetY);
        glEnd();

        printf("Drawing quad at %i,%i:%i,%i\n", (int)offsetX, (int)offsetY, (int)width, (int)height);

        glPopMatrix();

        glFinish();

        syscall(SYSCALL_SETGRAPHICSBUFFER, mesaBuffer);

        return;
    }

    syscall(SYSCALL_SETGRAPHICSBUFFER, buffer);
}

int ToastCreateCenteredGraphicsContext(int width, int height)
{
    if(usingCenteredGraphicsContext || width <= 0 || height <= 0)
    {
        return 0;
    }

    printf("ToastGraphics: Creating centered graphics context with size %ix%i\n", width, height);

    printf("ToastGraphics: Set graphics type to GUI\n");

    ToastSetGraphicsType(TOAST_GRAPHICS_TYPE_GUI);

    int bpp;

    ToastGetGraphicsSize(&graphicsWidth, &graphicsHeight, &bpp);

    printf("ToastGraphics: Got graphics size of %ix%ix%i\n", graphicsWidth, graphicsHeight, bpp);

    int bufferByteSize = sizeof(uint32_t) * graphicsWidth * graphicsHeight;

    uint32_t *mesaBuffer = (uint32_t *)malloc(bufferByteSize);

    memset(mesaBuffer, 128, bufferByteSize);

    printf("ToastGraphics: Allocated buffer of size %i\n", bufferByteSize);

    ToastSetGraphicsBuffer(mesaBuffer);

    OSMesaContext GLContext = OSMesaCreateContext(OSMESA_BGRA, NULL);

    if (!OSMesaMakeCurrent(GLContext, mesaBuffer, GL_UNSIGNED_BYTE, graphicsWidth, graphicsHeight))
    {
        printf("ToastGraphics: Failed to create OSMesa buffer\n");

        ToastSetGraphicsType(TOAST_GRAPHICS_TYPE_CONSOLE);

        free(mesaBuffer);

		return 0;
    }

    printf("ToastGraphics: Finalizing setup\n");

	OSMesaPixelStore(OSMESA_Y_UP, 0);

    glViewport(0, 0, graphicsWidth, graphicsHeight);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, graphicsWidth, graphicsHeight, 0, -1, 1);

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

    printf("ToastGraphics: Context created with size %ix%i and offset %.02fx%.02f and scale factor %i%%\n", width, height, offsetX, offsetY, (int)(scaleFactor * 100));

    return 1;
}

#include <toast/syscall.h>
#include <toast/graphics.h>
#include <stdlib.h>
#include <stdbool.h>
#include <GLES/gl.h>
#include <GL/osmesa.h>
#include <string.h>
#include <stdio.h>

static float texcoords[8] = {

    0, 0,
    0, 1,
    1, 1,
    1, 0
};

struct CenteredGraphicsContext
{
    int centeredGraphicsWidth;
    int centeredGraphicsHeight;
    int graphicsWidth;
    int graphicsHeight;
    GLuint texID;
    uint32_t *mesaBuffer;
    float scaledWidth;
    float scaledHeight;
    float offsetX;
    float offsetY;
    float vertices[8];

    OSMesaContext GLContext;
}graphicsContext;

bool usingCenteredGraphicsContext = false;

void ToastSetGraphicsType(int type)
{
    syscall(SYSCALL_SETGRAPHICSTYPE, type);
}

void ToastGetGraphicsSize(int *width, int *height, int *bpp)
{
    if(usingCenteredGraphicsContext)
    {
        *width = graphicsContext.centeredGraphicsWidth;
        *height = graphicsContext.centeredGraphicsHeight;
        *bpp = 32;

        return;
    }

    syscall(SYSCALL_GETGRAPHICSSIZE, width, height, bpp);
}

void ToastSetGraphicsBuffer(const void *buffer)
{
    if(usingCenteredGraphicsContext)
    {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
            graphicsContext.centeredGraphicsWidth, graphicsContext.centeredGraphicsHeight,
            GL_BGRA, GL_UNSIGNED_BYTE, buffer);

        glColor3f(1, 1, 1);

        glVertexPointer(2, GL_FLOAT, 0, graphicsContext.vertices);
        glTexCoordPointer(2, GL_FLOAT, 0, texcoords);

        glDrawArrays(GL_QUADS, 0, 4);

        glFinish();

        syscall(SYSCALL_SETGRAPHICSBUFFER, graphicsContext.mesaBuffer);

        return;
    }

    syscall(SYSCALL_SETGRAPHICSBUFFER, buffer);
}

void ClearGraphicsType()
{
    ToastSetGraphicsType(TOAST_GRAPHICS_TYPE_CONSOLE);
}

int ToastCreateCenteredGraphicsContext(int width, int height)
{
    if(usingCenteredGraphicsContext || width <= 0 || height <= 0)
    {
        return 0;
    }

    ToastSetGraphicsType(TOAST_GRAPHICS_TYPE_GUI);

    atexit(ClearGraphicsType);

    int bpp;

    ToastGetGraphicsSize(&graphicsContext.graphicsWidth, &graphicsContext.graphicsHeight, &bpp);

    printf("Graphics Size is: %dx%d (%d)\n", graphicsContext.graphicsWidth, graphicsContext.graphicsHeight, bpp);

    int bufferByteSize = sizeof(uint32_t) * graphicsContext.graphicsWidth * graphicsContext.graphicsHeight;

    graphicsContext.mesaBuffer = (uint32_t *)malloc(bufferByteSize);

    memset(graphicsContext.mesaBuffer, 128, bufferByteSize);

    ToastSetGraphicsBuffer(graphicsContext.mesaBuffer);

    printf("Making current\n");

    graphicsContext.GLContext = OSMesaCreateContext(OSMESA_BGRA, NULL);

    if (!OSMesaMakeCurrent(graphicsContext.GLContext, graphicsContext.mesaBuffer, GL_UNSIGNED_BYTE, graphicsContext.graphicsWidth, graphicsContext.graphicsHeight))
    {
        printf("OSMesa failed to make current\n");

		return 0;
    }

	OSMesaPixelStore(OSMESA_Y_UP, 0);

    printf("Set viewport\n");

    glViewport(0, 0, graphicsContext.graphicsWidth, graphicsContext.graphicsHeight);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0, graphicsContext.graphicsWidth, graphicsContext.graphicsHeight, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.5f, 0.5f, 0.5f, 0.0f);

    glShadeModel(GL_FLAT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_DITHER);
    glDisable(GL_COLOR_MATERIAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

    glEnable(GL_TEXTURE_2D);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glGenTextures(1, &graphicsContext.texID);
    glBindTexture(GL_TEXTURE_2D, graphicsContext.texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

    glClear(GL_COLOR_BUFFER_BIT);

    float scaleFactor = 1.0f;

    if(width > height)
    {
        scaleFactor = graphicsContext.graphicsWidth / (float)width;
    }
    else
    {
        scaleFactor = graphicsContext.graphicsHeight / (float)height;
    }

    if(height * scaleFactor > graphicsContext.graphicsHeight)
    {
        scaleFactor = graphicsContext.graphicsHeight / (float)height;
    }

    graphicsContext.scaledWidth = scaleFactor * width;
    graphicsContext.scaledHeight = scaleFactor * height;

    if(graphicsContext.graphicsWidth > graphicsContext.scaledWidth)
    {
        graphicsContext.offsetX = (graphicsContext.graphicsWidth - graphicsContext.scaledWidth) / 2;
    }

    if(graphicsContext.graphicsHeight > graphicsContext.scaledHeight)
    {
        graphicsContext.offsetY = (graphicsContext.graphicsHeight - graphicsContext.scaledHeight) / 2;
    }

    graphicsContext.centeredGraphicsWidth = width;
    graphicsContext.centeredGraphicsHeight = height;

    float vertices[8] = {

        graphicsContext.offsetX, graphicsContext.offsetY,
        graphicsContext.offsetX, graphicsContext.offsetY + graphicsContext.scaledHeight,
        graphicsContext.offsetX + graphicsContext.scaledWidth, graphicsContext.offsetY + graphicsContext.scaledHeight,
        graphicsContext.offsetX + graphicsContext.scaledWidth, graphicsContext.offsetY,
    };

    memcpy(graphicsContext.vertices, vertices, sizeof(vertices));

    usingCenteredGraphicsContext = true;

    printf("ToastGraphics: Context created with size %ix%i and offset %ix%i and scale factor %i%%\n", width, height,
        (int)graphicsContext.offsetX, (int)graphicsContext.offsetY, (int)(scaleFactor * 100));

    return 1;
}

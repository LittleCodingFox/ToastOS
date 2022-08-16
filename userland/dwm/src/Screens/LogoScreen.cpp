#include <GLES/gl.h>
#include <GL/osmesa.h>
#include <toast/input.h>
#include "../../ext-libs/stb/stb_image.h"

#include "../Screen.hpp"

static float fadeIn = 0;
static bool goingDown = false;
static bool firstFrame = true;

static uint32_t logoTextureID;

void DrawLogo(float delta);

Screen logoScreen = {
    .type = ScreenType::Logo,
    .draw = DrawLogo,
};

void DrawLogo(float delta)
{
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if(firstFrame)
    {
        firstFrame = false;

        int imageWidth, imageHeight, channelCount;
        uint8_t *data = stbi_load("/system/logo.png", &imageWidth, &imageHeight, &channelCount, 0);

        printf("Loading logo texture... %s\n", data ? "OK" : "FAIL");

        if(data != NULL)
        {
            glGenTextures(1, &logoTextureID);
            glBindTexture(GL_TEXTURE_2D, logoTextureID);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

            stbi_image_free(data);

            printf("Generated OpenGL texture\n");
        }
    }

    glBindTexture(GL_TEXTURE_2D, logoTextureID);

    glColor4f(1, 1, 1, fadeIn);

    glPushMatrix();

    glTranslatef(screenWidth / 2, screenHeight / 2, 0);

    glBegin(GL_QUADS);

    glVertex2f(-128, -128);
    glTexCoord2f(0, 0);

    glVertex2f(-128, 128);
    glTexCoord2f(0, 1);

    glVertex2f(128, 128);
    glTexCoord2f(1, 1);

    glVertex2f(128, -128);
    glTexCoord2f(1, 0);

    glEnd();

    glPopMatrix();

    glBindTexture(GL_TEXTURE_2D, 0);

    glPushMatrix();

    glTranslatef(mouseX, mouseY, 0);

    glBegin(GL_QUADS);

    if(mouseButtons & TOAST_INPUT_MOUSE_BUTTON_LEFT)
    {
        glColor3f(1, 1, 0);
    }
    else
    {
        glColor3f(1, 1, 1);
    }

    glVertex2f(0, 0);
    glVertex2f(0, 64);
    glVertex2f(64, 64);
    glVertex2f(64, 0);

    glEnd();

    glPopMatrix();

    if(goingDown)
    {
        fadeIn -= delta;

        if(fadeIn < 0)
        {
            fadeIn = 0;

            goingDown = false;
        }
    }
    else
    {
        fadeIn += delta;

        if(fadeIn >= 1)
        {
            fadeIn = 1;

            goingDown = true;
        }
    }
}

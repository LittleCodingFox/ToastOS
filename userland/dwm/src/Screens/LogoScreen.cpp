#include <GLES/gl.h>
#include <GL/osmesa.h>

#include "../Screen.hpp"

static float fadeIn = 0;
static bool goingDown = false;

void DrawLogo(float delta);

Screen logoScreen = {
    .type = ScreenType::Logo,
    .draw = DrawLogo,
};

void DrawLogo(float delta)
{
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(1, 1, 1, fadeIn);

    glPushMatrix();

    glTranslatef(screenWidth / 2, screenHeight / 2, 0);

    glBegin(GL_QUADS);

    glVertex2f(-128, -128);
    glVertex2f(-128, 128);
    glVertex2f(128, 128);
    glVertex2f(128, -128);

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

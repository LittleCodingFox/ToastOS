#include <GLES/gl.h>
#include <GL/osmesa.h>

#include "../Screen.hpp"

static float fadeIn = 0;
static bool goingDown = false;

void DrawMain(float delta);

Screen mainScreen = {
    .type = ScreenType::Main,
    .draw = DrawMain,
};

void DrawMain(float delta)
{
    glColor4f(1, 1, 1, 1);

    glPushMatrix();

    glTranslatef(screenWidth / 2 - 128, screenHeight / 2 - 128, 0);

    glBegin(GL_QUADS);

    glVertex2f(-128, -128);
    glVertex2f(-128, 128);
    glVertex2f(128, 128);
    glVertex2f(128, -128);

    glEnd();

    glPopMatrix();
}

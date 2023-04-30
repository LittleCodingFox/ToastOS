#include <GLES/gl.h>
#include <GL/osmesa.h>

#include "../App.hpp"
#include "../Screen.hpp"
#include "../Resources.hpp"

static float fadeIn = 0;
static bool goingDown = false;

bool firstFrame = true;

int iconSize = 64;
int iconOffset = iconSize + 20;

void DrawMain(float delta);

Screen mainScreen = {
    .type = ScreenType::Main,
    .draw = DrawMain,
};

Vertex vertices[4];

void DrawMain(float delta)
{
    if(firstFrame)
    {
        firstFrame = false;

        for(auto &app : *apps.get())
        {
            GLuint texture;

            if(LoadImage(app.icon, &texture, &app.iconWidth, &app.iconHeight))
            {
                app.iconTexture = texture;
            }
        }
    }

    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(1, 1, 1, 1);

    float x = 20, y = 20;

    for(auto &app : *apps.get())
    {
        GeometryForRect(x, y, iconSize, iconSize, vertices);

        glBindTexture(GL_TEXTURE_2D, app.iconTexture);

        glBegin(GL_QUADS);

        for(int i = 0; i < 4; i++)
        {
            glVertex2f(vertices[i].position.x, vertices[i].position.y);
            glTexCoord2f(vertices[i].uv.x, vertices[i].uv.y);
        }

        glEnd();

        y += iconOffset;
    }

    glPopMatrix();
}

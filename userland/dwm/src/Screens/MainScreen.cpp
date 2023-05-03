#include <GLES/gl.h>
#include <GL/osmesa.h>
#include <stdio.h>
#include "../App.hpp"
#include "../Screen.hpp"
#include "../Resources.hpp"

static float fadeIn = 0;
static bool goingDown = false;

bool firstFrame = true;

int iconSize = 64;
int iconOffset = iconSize + 20;

GLuint wallpaper = 0;
int wallpaperWidth = 0;
int wallpaperHeight = 0;
PointF wallpaperSize;
PointF wallpaperOffset;

GLuint cursor = 0;
int cursorWidth = 0;
int cursorHeight = 0;
float cursorScale = 0.05f;

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

        GLuint texture;

        if(LoadImage("/system/dwm/Resources/Wallpapers/adrian-infernus-unsplash.jpg", &wallpaper, &wallpaperWidth, &wallpaperHeight))
        {
            auto aspectRatio = wallpaperWidth / (float)wallpaperHeight;

            wallpaperSize = PointF(screenWidth * aspectRatio, screenHeight);

            wallpaperOffset = PointF((screenWidth - wallpaperSize.x) / 2, (screenHeight - wallpaperSize.y) / 2);

            printf("Wallpaper: %i, WallpaperSize: %f, %f; offset: %f, %f; width: %i, height: %i\n", wallpaper,
                wallpaperSize.x, wallpaperSize.y,
                wallpaperOffset.x, wallpaperOffset.y,
                wallpaperWidth, wallpaperHeight);
        }

        LoadImage("/system/dwm/Resources/Cursors/default.png", &cursor, &cursorWidth, &cursorHeight);

        for(auto &app : *apps.get())
        {
            if(LoadImage(app.icon, &texture, &app.iconWidth, &app.iconHeight))
            {
                app.iconTexture = texture;
            }
        }
    }

    glColor4f(1, 1, 1, 1);

    if(wallpaper != 0)
    {
        GeometryForRect(Rect(0, 0, wallpaperSize.x, wallpaperSize.y), vertices);

        glBindTexture(GL_TEXTURE_2D, wallpaper);

        glBegin(GL_QUADS);

        for(int i = 0; i < 4; i++)
        {
            glVertex2f(vertices[i].position.x, vertices[i].position.y);
            glTexCoord2f(vertices[i].uv.x, vertices[i].uv.y);
        }

        glEnd();
    }

    float x = 20, y = 20;

    for(auto &app : *apps.get())
    {
        GeometryForRect(Rect(x, y, iconSize, iconSize), vertices);

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

    GeometryForRect(Rect(mouseX, mouseY, cursorWidth * cursorScale, cursorHeight * cursorScale), vertices);

    glBindTexture(GL_TEXTURE_2D, cursor);

    glBegin(GL_QUADS);

    for(int i = 0; i < 4; i++)
    {
        glVertex2f(vertices[i].position.x, vertices[i].position.y);
        glTexCoord2f(vertices[i].uv.x, vertices[i].uv.y);
    }

    glEnd();
}

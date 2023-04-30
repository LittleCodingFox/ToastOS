#include <stdio.h>
#include <GLES/gl.h>
#include <GL/osmesa.h>
#include "../../ext-libs/stb/stb_image.h"
#include "Resources.hpp"

bool LoadImage(const string &path, GLuint *textureID, int *width, int *height)
{
    int channelCount = 0;
    uint8_t *data = stbi_load(path.data(), width, height, &channelCount, 0);

    if(data != NULL)
    {
        if(channelCount != 3 && channelCount != 4)
        {
            stbi_image_free(data);

            return false;
        }

        glGenTextures(1, textureID);
        glBindTexture(GL_TEXTURE_2D, *textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, channelCount == 3 ? GL_RGB8 : GL_RGBA8, *width, *height, 0, channelCount == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, data);

        SetImageFiltering(*textureID, ImageFilter::Linear);

        stbi_image_free(data);

        glBindTexture(GL_TEXTURE_2D, 0);

        return true;
    }

    return false;
}

void DestroyImage(GLuint textureID)
{
    glDeleteTextures(1, &textureID);
}

void SetImageFiltering(GLuint textureID, ImageFilter filter)
{
    glBindTexture(GL_TEXTURE_2D, textureID);

    switch(filter)
    {
        case ImageFilter::Nearest:

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            break;

        case ImageFilter::Linear:

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            break;
    }
}

void GeometryForRect(Rect rect, Vertex *vertices)
{
    vertices[0].position = PointF(rect.x, rect.y);
    vertices[1].position = PointF(rect.x, rect.y + rect.height);
    vertices[2].position = PointF(rect.x + rect.width, rect.y + rect.height);
    vertices[3].position = PointF(rect.x + rect.width, rect.y);

    vertices[0].uv = PointF(0, 1);
    vertices[1].uv = PointF(1, 1);
    vertices[2].uv = PointF(1, 0);
    vertices[3].uv = PointF(0, 0);
}

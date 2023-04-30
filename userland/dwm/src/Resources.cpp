#include <GLES/gl.h>
#include <GL/osmesa.h>
#include "../../ext-libs/stb/stb_image.h"
#include "Resources.hpp"

bool LoadImage(const string &path, GLuint *textureID, int *width, int *height)
{
    int channelCount = 0;
    uint8_t *data = stbi_load(path.data(), width, height, &channelCount, 4);

    if(data != NULL)
    {
        glGenTextures(1, textureID);
        glBindTexture(GL_TEXTURE_2D, *textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, *width, *height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

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

void GeometryForRect(int x, int y, int width, int height, Vertex *vertices)
{
    vertices[0].position = PointF(x, y);
    vertices[1].position = PointF(x, y + height);
    vertices[2].position = PointF(x + width, y + height);
    vertices[3].position = PointF(x + width, y);

    vertices[0].uv = PointF(0, 1);
    vertices[1].uv = PointF(1, 1);
    vertices[2].uv = PointF(1, 0);
    vertices[3].uv = PointF(0, 0);
}

#pragma once
#include "frgdef.hpp"

struct PointF
{
    float x, y;

    PointF() : x(0), y(0) {}
    PointF(float x, float y) : x(x), y(y) {}
};

struct Vertex
{
    PointF position;
    PointF uv;
};

bool LoadImage(const string &path, GLuint *textureID, int *width, int *height);
void FreeImage(GLuint textureID);

void GeometryForRect(int x, int y, int width, int height, Vertex *vertices);

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

class Rect
{
public:
    float x, y;
    float width, height;

    Rect() : x(0), y(0), width(0), height(0) {}

    Rect(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) {}
};

enum class ImageFilter
{
    Nearest,
    Linear,
};

bool LoadImage(const string &path, GLuint *textureID, int *width, int *height);
void FreeImage(GLuint textureID);
void SetImageFiltering(GLuint textureID, ImageFilter filter);

void GeometryForRect(Rect rect, Vertex *vertices);

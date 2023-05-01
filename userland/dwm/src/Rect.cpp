#include <GLES/gl.h>
#include <GL/osmesa.h>
#include "Resources.hpp"

bool Rect::Contains(PointF p)
{
    return x <= p.x && x + width >= p.x && y <= p.y && y + height >= p.y;
}

bool Rect::Overlaps(const Rect &other)
{
    return x <= other.x + other.width && x + width >= other.x && y <= other.y + other.height && y + height >= other.y;
}

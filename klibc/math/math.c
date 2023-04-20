#include <math.h>

int min(int a, int b)
{
    return a < b ? a : b;
}

int max(int a, int b)
{
    return a > b ? a : b;
}

int powi(int x, int y)
{
    unsigned int n = 1;

    while (y--) {
        n *= x;
    }

    return n;
}

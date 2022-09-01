#include "random.h"

static uint64_t next = 0;

uint64_t random()
{
    next = next * 1103515245 + 12345;

    return (next / 65536) % 32768;
}

void seedRandom(uint64_t seed)
{
    next = seed;
}

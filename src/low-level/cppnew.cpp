#include <stdint.h>
#include <stdlib.h>

void* operator new(size_t size)
{
    return malloc(size);
}

void* operator new[](size_t size)
{
    return malloc(size);
}

void operator delete(void* p)
{
    free(p);
}

void operator delete(void *p, uint64_t length)
{
    free(p);
}

void operator delete[](void *p)
{
    free(p);
}

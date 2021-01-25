#include <stdlib.h>
#include "liballoc/liballoc.h"

void *malloc(size_t size)
{
    return kmalloc(size);
}

void *calloc(size_t nmemb, size_t size)
{
    return kcalloc(nmemb, size);
}

void *realloc(void *ptr, size_t size)
{
    return krealloc(ptr, size);
}

void *free(void *ptr)
{
    kfree(ptr);
}

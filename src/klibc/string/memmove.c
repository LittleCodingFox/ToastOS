#include "string.h"
#include <stddef.h>
#include <stdint.h>

void *memmove(void *dst, const void *src, size_t size)
{
	unsigned char* dstptr = (unsigned char*) dst;
	const unsigned char* srcptr = (const unsigned char*) src;

	if (dst < src)
    {
		for (size_t i = 0; i < size; i++)
        {
			dstptr[i] = srcptr[i];
        }
	}
    else
    {
		for (size_t i = size; i != 0; i--)
        {
			dstptr[i-1] = srcptr[i-1];
        }
	}

	return dstptr;
}

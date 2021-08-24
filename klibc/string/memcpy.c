#include "string.h"
#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t size)
{
    unsigned char* dstptr = (unsigned char*) dst;
	const unsigned char* srcptr = (const unsigned char*) src;

	for (size_t i = 0; i < size; i++)
    {
		dstptr[i] = srcptr[i];
    }
    
	return dstptr;
}

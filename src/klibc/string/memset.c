#include "string.h"
#include <stddef.h>

void *memset(void *ptr, int val, size_t size)
{
	unsigned char* buf = (unsigned char*) ptr;

	for (size_t i = 0; i < size; i++)
    {
		buf[i] = (unsigned char) val;
    }

	return ptr;
}

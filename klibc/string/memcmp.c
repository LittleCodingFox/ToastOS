#include "string.h"
#include <stddef.h>
#include <stdint.h>

int memcmp(const void *src1, const void *src2, size_t size)
{
    if(!size)
    {
        return 0;
    }

    const unsigned char* a = (const unsigned char*) src1;
	const unsigned char* b = (const unsigned char*) src2;

	for (size_t i = 0; i < size; i++)
    {
		if (a[i] < b[i])
        {
			return -1;
        }
		else if (b[i] < a[i])
        {
			return 1;
        }
	}
    
	return 0;
}

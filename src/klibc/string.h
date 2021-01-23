#ifndef STRING_H
#define STRING_H

#include "include/kernel.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

void *memcpy(void *dst, const void *src, size_t size);
void *memset(void *ptr, int val, size_t size);
void *memmove(void *dst, const void *src, size_t size);
int memcmp(const void *src1, const void *src2, size_t size);

#ifdef __cplusplus
}
#endif

#endif

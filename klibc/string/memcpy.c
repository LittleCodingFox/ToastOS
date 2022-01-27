#include "string.h"
#include "low-level/sse/sse.hpp"
#include "include/kernel.h"
#include <stddef.h>

void *memcpy_basic(void *dst, const void *src, size_t size)
{
	asm volatile ("rep movsb"
          			: "=D" (dst),
                  "=S" (src),
                  "=c" (size)
                : "0" (dst),
                  "1" (src),
                  "2" (size)
                : "memory");

	return dst;
}

void *memcpy(void *dst, const void *src, size_t size)
{
  if(SSEEnabled() && ALIGNED(dst, 16) && ALIGNED(src, 16))
  {
    sse_memcpy(dst, src, size);

    return dst;
  }

  return memcpy_basic(dst, src, size);
}

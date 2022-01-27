#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#if __cplusplus
extern "C" {
#endif

void EnableSSE();
bool SSEEnabled();

void sse_memcpy(void *dst, const void *src, size_t size);

#if __cplusplus
}
#endif

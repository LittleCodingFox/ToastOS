#pragma once

#include <stdint.h>

#if __cplusplus
extern "C"
{
#endif

#define SYSCALL_WRITE 1
#define SYSCALL_READ 2

extern int64_t syscall(uint64_t id, ...);

#if __cplusplus
}
#endif

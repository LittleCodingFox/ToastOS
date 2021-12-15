#pragma once

#include <stdint.h>
#include "low-level/syscall/syscall.hpp"

#if __cplusplus
extern "C"
{
#endif

extern int64_t syscall(uint64_t id, ...);

#if __cplusplus
}
#endif

#include "../stdio.h"

#if IS_LIBC
#   include "../sys/syscall.h"
#else
#   include "printf/printf.h"
#   include "low-level/keyboard/Keyboard.hpp"
#endif

int open(const char *path, uint32_t perms)
{
#if IS_LIBC
    return syscall(SYSCALL_OPEN, path, perms);
#else
    return 0;
#endif
}
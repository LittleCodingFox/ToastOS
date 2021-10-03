#include "../stdio.h"

#if IS_LIBC
#   include "../sys/syscall.h"
#else
#   include "printf/printf.h"
#   include "low-level/keyboard/Keyboard.hpp"
#endif

int close(int fd)
{
#if IS_LIBC
    return syscall(SYSCALL_CLOSE, fd);
#else
    return 0;
#endif
}
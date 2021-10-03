#include "../stdio.h"

#if IS_LIBC
#   include "../sys/syscall.h"
#else
#   include "printf/printf.h"
#   include "low-level/keyboard/Keyboard.hpp"
#endif

uint64_t seek(int fd, uint64_t offset, int whence)
{
#if IS_LIBC
    return syscall(SYSCALL_SEEK, fd, offset, whence);
#else
    return 0;
#endif
}
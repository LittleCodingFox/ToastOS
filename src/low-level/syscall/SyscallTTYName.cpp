#include <dirent.h>
#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "fcntl.h"
#include "filesystems/VFS.hpp"
#include "user/UserAccess.hpp"

int64_t SyscallTTYName(InterruptStack *stack)
{
    int fd = stack->rsi;
    char *buf = (char *)stack->rdx;
    size_t size = (size_t)stack->rcx;

    (void)fd;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: ttyname fd: %i; buf: %p; size: %lu", fd, (void *)buf, size);
#endif

    if(!SanitizeUserPointer(buf))
    {
        return ENOENT;
    }

    static const char *name = "tty";

    size_t length = strlen(name) + 1 < size ? size - 1 : strlen(name);

    memcpy(buf, name, length);

    buf[length] = '\0';

    return length;
}

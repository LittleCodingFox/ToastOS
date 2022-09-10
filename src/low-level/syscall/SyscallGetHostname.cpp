#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"
#include "user/UserAccess.hpp"

int64_t SyscallGetHostname(InterruptStack *stack)
{
    char *buffer = (char *)stack->rsi;
    size_t size = (size_t)stack->rdx;

    if(!SanitizeUserPointer(buffer))
    {
        return -EINVAL;
    }

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: gethostname %p %llu", buffer, size);
#endif

    static const char *hostname = "toast";

    size_t length = strlen(hostname) + 1 < size ? size - 1 : strlen(hostname);

    memcpy(buffer, hostname, length);

    buffer[length] = '\0';

    return length;
}

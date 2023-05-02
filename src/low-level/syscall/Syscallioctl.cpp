#include <dirent.h>
#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "fcntl.h"
#include "filesystems/VFS.hpp"
#include "user/UserAccess.hpp"

int64_t Syscallioctl(InterruptStack *stack)
{
    int fd = stack->rsi;
    unsigned long request = stack->rdx;
    void *arg = (void *)stack->rcx;
    int *result = (int *)stack->r8;

    (void)fd;
    (void)request;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: ioctl fd: %i; request: %lu; arg: %p; result: %p", fd, request, arg, (void *)result);
#endif

    if(!SanitizeUserPointer(result) || !SanitizeUserPointer(arg))
    {
        return -EINVAL;
    }

    //TODO

    return 0;
}

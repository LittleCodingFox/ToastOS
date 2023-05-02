#include <dirent.h>
#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "fcntl.h"
#include "filesystems/VFS.hpp"
#include "user/UserAccess.hpp"

int64_t SyscallFcntl(InterruptStack *stack)
{
    int fd = stack->rsi;
    int request = stack->rdx;
    size_t arg = (size_t)stack->rcx;
    int *result = (int *)stack->r8;

    if(!SanitizeUserPointer(result))
    {
        return ENOENT;
    }

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: fcntl fd: %i; request: %x; arg: %lu; result: %p", fd, request, arg, (void *)result);
#endif

    auto process = processManager->CurrentProcess();

    if(process == NULL || process->isValid == false)
    {
        return ENOENT;
    }

    switch(request)
    {
        case F_DUPFD:

            *result = process->info->DuplicateFD(fd, arg);

            break;

        case F_DUPFD_CLOEXEC:

            break;

        case F_GETFD:

            *result = 0;

            break;

        case F_SETFD:

            break;

        case F_GETFL:

            if(fd == 0)
            {
                *result = O_WRONLY;
            }
            else if(fd == 1 || fd == 2) //stdout and stderr
            {
                *result = O_RDONLY;
            }
            else
            {
                FILE_HANDLE handle = fd - 3;

                uint32_t flags = vfs->FileFlags(handle);

                if(flags == 0)
                {
                    return EBADF;
                }

                *result = flags;
            }

            break;

        case F_SETFL:

            if(fd >= 3)
            {
                FILE_HANDLE handle = fd - 3;

                uint32_t flags = vfs->FileFlags(handle);

                if(flags == 0)
                {
                    return EBADF;
                }

                #define SET_FLAG(flag)\
                if((arg & flag) == 0)\
                {\
                    flags |= flag;\
                }\
                else\
                {\
                    flags &= ~flag;\
                }

                SET_FLAG(O_APPEND);
                SET_FLAG(O_NONBLOCK);

                vfs->SetFileFlags(handle, flags);

                *result = flags;
            }

            break;
    }

    return 0;
}
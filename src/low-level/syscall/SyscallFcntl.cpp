#include <dirent.h>
#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "fcntl.h"
#include "filesystems/VFS.hpp"

using namespace FileSystem;

int64_t SyscallFcntl(InterruptStack *stack)
{
    int fd = stack->rsi;
    int request = stack->rdx;
    size_t arg = (size_t)stack->rcx;
    int *result = (int *)stack->r8;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: fcntl fd: %i; request: %i; arg: %llu; result: %p", fd, request, arg, result);
#endif

    switch(request)
    {
        case F_DUPFD:

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
#include "syscall.hpp"
#include "interrupts/Interrupts.hpp"
#include "printf/printf.h"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"

using namespace FileSystem;

size_t SyscallWrite(InterruptStack *stack)
{
    int fd = stack->rsi;
    const void *buffer = (const void *)stack->rdx;
    size_t count = (size_t)stack->rcx;

    if(fd == 0) //stdin
    {
        //TODO
    }
    else if(fd == 1) //stdout
    {
        printf("%.*s", count, buffer);

        return count;
    }
    else if(fd == 2) //stderr
    {
        printf("%.*s", count, buffer);

        return count;
    }
    else if(fd >= 3) //actual files
    {
        FILE_HANDLE handle = fd - 3;

        if(vfs->FileType(handle) != FILE_HANDLE_UNKNOWN)
        {
            return -ENOENT;
        }

        return vfs->WriteFile(handle, buffer, count);
    }

    return 0;
}
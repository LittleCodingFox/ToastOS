#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"
#include "fcntl.h"

using namespace FileSystem;

int64_t SyscallSeek(InterruptStack *stack)
{
    int fd = (int)stack->rsi;
    uint64_t offset = stack->rdx;
    int whence = (int)stack->rcx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: seek fd: %i; offset: %llu; whence: %x", fd, offset, whence);
#endif

    if(fd < 3)
    {
        return -ESPIPE;
    }

    FILE_HANDLE handle = fd - 3;

    if(vfs->FileType(handle) == FILE_HANDLE_UNKNOWN)
    {
        return -EBADF;
    }

    switch(whence)
    {
        case SEEK_SET:

            if(offset > vfs->FileLength(handle))
            {
                return -EINVAL;
            }

            return vfs->SeekFile(handle, offset);

        case SEEK_CUR:

            if(vfs->FileOffset(handle) + offset > vfs->FileLength(handle))
            {
                return -EINVAL;
            }

            return vfs->SeekFile(handle, vfs->FileOffset(handle) + offset);

        case SEEK_END:

            return vfs->SeekFileEnd(handle);
    }

    return -EINVAL;
}
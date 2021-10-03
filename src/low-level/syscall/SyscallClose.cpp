#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"

using namespace FileSystem;

int64_t SyscallClose(InterruptStack *stack)
{
    int fd = (int)stack->rsi;

    if(fd < 3)
    {
        return -EPERM;
    }

    FILE_HANDLE handle = fd - 3;

    if(vfs.FileType(handle) == FILE_HANDLE_UNKNOWN)
    {
        return -EBADF;
    }

    vfs.CloseFile(handle);

    return 0;
}
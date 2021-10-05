#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"

using namespace FileSystem;

int64_t SyscallOpen(InterruptStack *stack)
{
    const char *path = (const char *)stack->rsi;
    uint32_t flags = (uint32_t)stack->rdx;

    FILE_HANDLE handle = vfs.OpenFile(path);

    if(handle == INVALID_FILE_HANDLE)
    {
        return -ENOENT;
    }

    return handle + 3; //Ensure we're above the base fds for stdin, stdout, and stderr
}
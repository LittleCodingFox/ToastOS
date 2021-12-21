#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"

using namespace FileSystem;

int64_t SyscallFStat(InterruptStack *stack)
{
    FileSystemStat *stat = (FileSystemStat *)stack->rsi;
    int fd = (int)stack->rdx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: fstat stat: %p; fd: %i", stat, fd);
#endif

    if(fd < 3)
    {
        return -1;
    }

    FILE_HANDLE handle = fd - 3;

    if(vfs->FileType(handle) == FILE_HANDLE_UNKNOWN)
    {
        return -1;
    }

    *stat = vfs->Stat(handle);

    return 0;
}
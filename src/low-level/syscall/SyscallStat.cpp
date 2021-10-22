#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"

using namespace FileSystem;

int64_t SyscallStat(InterruptStack *stack)
{
    FileSystemStat *stat = (FileSystemStat *)stack->rsi;
    const char *path = (const char *)stack->rdx;

    FILE_HANDLE handle = vfs->OpenFile(path);

    if(vfs->FileType(handle) == FILE_HANDLE_UNKNOWN)
    {
        return -1;
    }

    *stat = vfs->Stat(handle);

    vfs->CloseFile(handle);

    return 0;
}
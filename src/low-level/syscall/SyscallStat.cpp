#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"
#include "fcntl.h"

using namespace FileSystem;

int64_t SyscallStat(InterruptStack *stack)
{
    FileSystemStat *stat = (FileSystemStat *)stack->rsi;
    const char *path = (const char *)stack->rdx;

    DEBUG_OUT("Stat for %s", path);

    FILE_HANDLE handle = vfs->OpenFile(path, O_RDONLY, globalProcessManager->CurrentProcess());

    if(vfs->FileType(handle) == FILE_HANDLE_UNKNOWN)
    {
        DEBUG_OUT("Stat is unknown", 0);

        return -1;
    }

    *stat = vfs->Stat(handle);

    vfs->CloseFile(handle);

    DEBUG_OUT("Stat OK!", 0);

    return 0;
}
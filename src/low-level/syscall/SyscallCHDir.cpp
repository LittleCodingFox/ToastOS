#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"
#include "kernel.h"

int64_t SyscallCHDir(InterruptStack *stack)
{
    char *path = (char *)stack->rsi;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: chdir path: %s", path);
#endif

    auto process = globalProcessManager->CurrentProcess();

    if(process == NULL || process->isValid == false)
    {
        return ENOENT;
    }

    auto handle = vfs->OpenFile(path, 0, process->info);

    if(vfs->FileType(handle) != FILE_HANDLE_DIRECTORY)
    {
        if(vfs->FileType(handle) == FILE_HANDLE_UNKNOWN)
        {
            return ENOENT;
        }

        vfs->CloseFile(handle);

        return ENOTDIR;
    }

    process->info->cwd = string("/") + vfs->GetFilePath(handle);

    if(process->info->cwd[process->info->cwd.size() - 1] != '/')
    {
        process->info->cwd += "/";
    }

    vfs->CloseFile(handle);

    return 0;
}

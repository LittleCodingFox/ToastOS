#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"
#include "kernel.h"

int64_t SyscallExecve(InterruptStack *stack)
{
    const char *path = (const char *)stack->rsi;
    const char **argv = (const char **)stack->rdx;
    const char **envp = (const char **)stack->rcx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: execve path: %s argv: %p envp: %p", path, argv, envp);
#endif

    auto process = globalProcessManager->CurrentProcess();

    if(process == NULL || process->isValid == false)
    {
        return ENOENT;
    }

    FILE_HANDLE handle = vfs->OpenFile(path, 0, process->info);

    if(handle == INVALID_FILE_HANDLE)
    {
        DEBUG_OUT("Execve: Failed to open %s!", path);

        return ENOENT;
    }

    auto length = vfs->FileLength(handle);

    uint8_t *buffer = new uint8_t[length];

    if(vfs->ReadFile(handle, buffer, length) != length)
    {
        DEBUG_OUT("Execve: Failed to read data for %s!", path);

        delete [] buffer;

        vfs->CloseFile(handle);

        return EIO;
    }

    auto pair = globalProcessManager->LoadImage(buffer, path, argv, envp, process->info->cwd.data(), PROCESS_PERMISSION_USER, process->info->ID);

    pair->process->ppid = process->info->ppid;
    pair->process->uid = process->info->uid;
    pair->process->gid = process->info->gid;

    delete [] buffer;

    vfs->CloseFile(handle);

    globalProcessManager->Exit(0, true);

    return 0;
}
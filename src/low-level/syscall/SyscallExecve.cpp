#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"
#include "kernel.h"
#include "user/UserAccess.hpp"

int64_t SyscallExecve(InterruptStack *stack)
{
    const char *path = (const char *)stack->rsi;
    const char **argv = (const char **)stack->rdx;
    const char **envp = (const char **)stack->rcx;

    if(!SanitizeUserPointer(path) || !SanitizeUserPointer(argv) || !SanitizeUserPointer(envp))
    {
        return ENOENT;
    }

    const char **arg = argv;

    while(*arg != NULL)
     {
        if(!SanitizeUserPointer(*arg))
        {
            return ENOENT;
        }

        arg++;
     }

    const char **env = envp;

    while(*env != NULL)
     {
        if(!SanitizeUserPointer(*env))
        {
            return ENOENT;
        }

        env++;
     }

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: execve path: %s argv: %p envp: %p", path, argv, envp);
#endif

    bool interruptsEnabled = interrupts.InterruptsEnabled();

    if(interruptsEnabled)
    {
        interrupts.DisableInterrupts();
    }

    auto process = processManager->CurrentProcess();

    if(process == NULL || process->isValid == false)
    {
        if(interruptsEnabled)
        {
            interrupts.EnableInterrupts();
        }

        return ENOENT;
    }

    int error = 0;

    FILE_HANDLE handle = vfs->OpenFile(path, 0, process->info, &error);

    if(handle == INVALID_FILE_HANDLE)
    {
        DEBUG_OUT("Execve: Failed to open %s! (error: %x)", path, error);

        if(interruptsEnabled)
        {
            interrupts.EnableInterrupts();
        }

        return ENOENT;
    }

    if(error != 0)
    {
        if(interruptsEnabled)
        {
            interrupts.EnableInterrupts();
        }

        return error;
    }

    auto length = vfs->FileLength(handle);

    uint8_t *buffer = new uint8_t[length];

    if(vfs->ReadFile(handle, buffer, length, &error) != length)
    {
        DEBUG_OUT("Execve: Failed to read data for %s!", path);

        if(interruptsEnabled)
        {
            interrupts.EnableInterrupts();
        }

        delete [] buffer;

        vfs->CloseFile(handle);

        return error != 0 ? error : EIO;
    }

    auto pair = processManager->LoadImage(buffer, path, argv, envp, process->info->cwd.data(), PROCESS_PERMISSION_USER, process->info->ID);

    pair->process->ppid = process->info->ppid;
    pair->process->uid = process->info->uid;
    pair->process->gid = process->info->gid;
    pair->process->fds = process->info->fds;
    pair->process->pipes = process->info->pipes;
    pair->process->fdCounter = process->info->fdCounter;

    pair->process->IncreaseFDRefs();

    delete [] buffer;

    vfs->CloseFile(handle);

    processManager->Exit(0, true);

    return 0;
}
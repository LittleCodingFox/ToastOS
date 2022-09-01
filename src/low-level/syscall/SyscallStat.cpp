#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"
#include "fcntl.h"
#include "user/UserAccess.hpp"

int64_t SyscallStat(InterruptStack *stack)
{
    struct stat *stat = (struct stat *)stack->rsi;
    const char *path = (const char *)stack->rdx;

    if(!SanitizeUserPointer(stat) || !SanitizeUserPointer(path))
    {
        return -1;
    }

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: stat stat: %p path %s", stat, path);
#endif

    auto currentProcess = processManager->CurrentProcess();

    if(currentProcess == NULL || currentProcess->isValid == false)
    {
        return -1;
    }

    int error = 0;

    FILE_HANDLE handle = vfs->OpenFile(path, O_RDONLY, currentProcess->info, &error);
    
    if(vfs->FileType(handle) == FILE_HANDLE_UNKNOWN || error != 0)
    {
        //DEBUG_OUT(error == 0 ? "Stat is unknown" : "Stat Error: %i", error);

        return -1;
    }

    *stat = vfs->Stat(handle, &error);

    vfs->CloseFile(handle);

    if(error != 0)
    {
        //DEBUG_OUT("Stat Error: %i", error);

        return -error;
    }

    //DEBUG_OUT("Stat OK!", 0);

    return 0;
}
#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"
#include "fcntl.h"

int64_t SyscallStat(InterruptStack *stack)
{
    struct stat *stat = (struct stat *)stack->rsi;
    const char *path = (const char *)stack->rdx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: stat stat: %p path %s", stat, path);
#endif

    auto currentProcess = globalProcessManager->CurrentProcess();

    if(currentProcess == NULL || currentProcess->isValid == false)
    {
        return -1;
    }

    FILE_HANDLE handle = vfs->OpenFile(path, O_RDONLY, currentProcess->info);
    
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
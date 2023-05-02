#include "syscall.hpp"
#include "process/Process.hpp"
#include "interrupts/Interrupts.hpp"
#include "debug.hpp"
#include "errno.h"
#include "user/UserAccess.hpp"

size_t SyscallRename(InterruptStack *stack)
{
    const char *path = (const char *)stack->rsi;
    const char *newPath = (const char *)stack->rdx;

    (void)path;
    (void)newPath;

    if(!SanitizeUserPointer(path) || !SanitizeUserPointer(newPath))
    {
        return 0;
    }

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: rename path: %p; newPath: %p", path, newPath);
#endif

    auto process = processManager->CurrentProcess();

    if(process == nullptr || process->isValid == false)
    {
        return ENOENT;
    }

    if(vfs->Rename(path, newPath, process->info) == false)
    {
        return ENOENT;
    }

    return 0;
}
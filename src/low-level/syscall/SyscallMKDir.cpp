#include "syscall.hpp"
#include "process/Process.hpp"
#include "interrupts/Interrupts.hpp"
#include "printf/printf.h"
#include "debug.hpp"
#include "errno.h"
#include "user/UserAccess.hpp"

size_t SyscallMKDir(InterruptStack *stack)
{
    const char *path = (const char *)stack->rsi;
    mode_t mode = (mode_t)stack->rdx;

    (void)path;
    (void)mode;

    if(!SanitizeUserPointer(path))
    {
        return 0;
    }

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: mkdir path: %p; mode: %x", path, mode);
#endif

    auto process = processManager->CurrentProcess();

    if(process == nullptr || process->isValid == false)
    {
        return ENOENT;
    }

    if(vfs->MakeDir(path, mode, process->info) == false)
    {
        return ENOENT;
    }

    return 0;
}
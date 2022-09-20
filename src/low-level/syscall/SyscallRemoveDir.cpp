#include "syscall.hpp"
#include "process/Process.hpp"
#include "interrupts/Interrupts.hpp"
#include "printf/printf.h"
#include "debug.hpp"
#include "errno.h"
#include "user/UserAccess.hpp"

size_t SyscallRemoveDir(InterruptStack *stack)
{
    const char *path = (const char *)stack->rsi;

    (void)path;

    if(!SanitizeUserPointer(path))
    {
        return 0;
    }

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: rmdir path: %p", path);
#endif

    auto process = processManager->CurrentProcess();

    if(process == nullptr || process->isValid == false)
    {
        return ENOENT;
    }

    if(vfs->RemoveDir(path, process->info) == false)
    {
        return ENOENT;
    }

    return 0;
}
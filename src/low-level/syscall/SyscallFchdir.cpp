#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "signal.h"
#include "user/UserAccess.hpp"

int64_t SyscallFchdir(InterruptStack *stack)
{
    int fd = stack->rsi;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: fchdir fd: %i", fd);
#endif

    auto process = processManager->CurrentProcess();

    if(process == NULL || process->isValid == false)
    {
        return 0;
    }

    auto processFD = process->info->GetFD(fd);

    if(processFD == NULL ||
        processFD->isValid == false ||
        processFD->type != PROCESS_FD_HANDLE ||
        processFD->impl == NULL)
    {
        return ENOENT;
    }

    auto handle = (ProcessFDVFS *)processFD->impl;

    auto cwd = vfs->GetFilePath(handle->handle);

    if(cwd.size() == 0)
    {
        return -ENOENT;
    }

    process->info->cwd = cwd;

    return 0;
}

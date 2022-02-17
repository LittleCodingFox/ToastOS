#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"

int64_t SyscallFStat(InterruptStack *stack)
{
    struct stat *stat = (struct stat *)stack->rsi;
    int fd = (int)stack->rdx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: fstat stat: %p; fd: %i", stat, fd);
#endif

    auto current = globalProcessManager->CurrentProcess();

    if(current == NULL || current->isValid == false)
    {
        return -EBADF;
    }

    auto procfd = current->info->GetFD(fd);

    if(procfd == NULL)
    {
        return -EBADF;
    }

    int error = 0;

    *stat = procfd->impl->Stat(&error);

    if(error != 0)
    {
        return -error;
    }

    return 0;
}
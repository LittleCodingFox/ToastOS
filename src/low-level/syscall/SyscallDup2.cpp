#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "kernel.h"

int64_t SyscallDup2(InterruptStack *stack)
{
    int fd = stack->rsi;
    int flags = stack->rdx;
    int newfd = stack->rcx;

    (void)flags;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: dup2 fd: %i flags: %x newfd: %i", fd, flags, newfd);
#endif

    auto process = globalProcessManager->CurrentProcess();

    if(process == NULL || process->isValid == false)
    {
        return ENOENT;
    }

    auto fdInstance = process->info->GetFD(fd);

    if(fdInstance == NULL)
    {
        return EBADF;
    }

    process->info->DuplicateFD(fd, newfd);

    return 0;
}

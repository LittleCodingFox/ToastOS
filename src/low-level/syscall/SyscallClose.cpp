#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"

int64_t SyscallClose(InterruptStack *stack)
{
    int fd = (int)stack->rsi;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: close fd: %i", fd);
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

    procfd->isValid = false;

    return 0;
}
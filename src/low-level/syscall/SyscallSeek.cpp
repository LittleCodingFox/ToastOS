#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"
#include "fcntl.h"

int64_t SyscallSeek(InterruptStack *stack)
{
    int fd = (int)stack->rsi;
    uint64_t offset = stack->rdx;
    int whence = (int)stack->rcx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: seek fd: %i; offset: %lu; whence: %x", fd, offset, whence);
#endif

    auto current = processManager->CurrentProcess();

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

    auto seek = procfd->impl->Seek(offset, whence, &error);

    if(error != 0)
    {
        return -error;
    }

    return seek;
}
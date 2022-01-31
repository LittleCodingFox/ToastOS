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
    DEBUG_OUT("Syscall: seek fd: %i; offset: %llu; whence: %x", fd, offset, whence);
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

    return procfd->impl->Seek(offset, whence);
}
#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "input/InputSystem.hpp"
#include "debug.hpp"
#include "errno.h"
#include "ctype.h"

extern bool InputEnabled;

int64_t SyscallRead(InterruptStack *stack)
{
    int fd = stack->rsi;
    void *buffer = (void *)stack->rdx;
    size_t count = (size_t)stack->rcx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: Read fd: %i buffer: %p count: %llu", fd, buffer, count);
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

    uint64_t read = procfd->impl->Read(buffer, count, &error);

    if(error != 0)
    {
        return -error;
    }

    return read;
}
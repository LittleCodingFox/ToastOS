#include "syscall.hpp"
#include "interrupts/Interrupts.hpp"
#include "printf/printf.h"
#include "filesystems/VFS.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"

size_t SyscallWrite(InterruptStack *stack)
{
    int fd = stack->rsi;
    const void *buffer = (const void *)stack->rdx;
    size_t count = (size_t)stack->rcx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: write fd: %i buffer: %p count: %llu", fd, buffer, count);
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

    uint64_t write = procfd->impl->Write(buffer, count, &error);

    if(error != 0)
    {
        return -error;
    }

    return write;
}
#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "abi-bits/socket.h"

int64_t SyscallSocket(InterruptStack *stack)
{
    int domain = stack->rsi;
    int type = stack->rdx;
    int protocol = stack->rcx;

#if KERNEL_DEBUG_SYSCALLS || 1
    DEBUG_OUT("Syscall: socket domain %x type %x protocol %x", domain, type, protocol);
#endif

    auto process = processManager->CurrentProcess();

    if(process == NULL || process->isValid == false)
    {
        return 0;
    }

    if(domain != PF_UNIX || type != SOCK_DGRAM)
    {
        return -EINVAL;
    }

    auto fd = process->info->AddFD(ProcessFDType::Socket, new ProcessFDSocket(domain, type, protocol, 0));

    return fd;
}

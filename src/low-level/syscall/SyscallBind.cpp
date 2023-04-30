#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "sys/un.h"

int64_t SyscallBind(InterruptStack *stack)
{
    int sockfd = stack->rsi;
    const struct sockaddr *addr_ptr = (const struct sockaddr *)stack->rdx;
    socklen_t addrlen = (socklen_t)stack->rcx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: bind sockfd %d addr_ptr %p addrlen %i", sockfd, addr_ptr, addrlen);
#endif

    auto process = processManager->CurrentProcess();

    if(process == NULL || process->isValid == false)
    {
        return 0;
    }

    auto fd = process->info->GetFD(sockfd);

    if(fd == NULL || fd->impl == NULL || fd->isValid == false)
    {
        return -EBADF;
    }

    if(fd->type != ProcessFDType::Socket)
    {
        return -ENOTSOCK;
    }

    auto socket = (ProcessFDSocket *)fd->impl;

    if(socket != NULL && socket->socket != NULL)
    {
        return socket->socket->Bind(addr_ptr, addrlen, process->info);
    }

    return 0;
}

#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "abi-bits/socket.h"
#include "sys/un.h"

int64_t SyscallRecvMsg(InterruptStack *stack)
{
    int sockfd = stack->rsi;
    struct msghdr *hdr = (struct msghdr *)stack->rdx;
    int flags = stack->rcx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: recvmsg sockfd %d hdr %p flags %x", sockfd, hdr, flags);
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

    if((flags & MSG_PEEK) == MSG_PEEK)
    {
        return socket->PeekMessage()
    }

    if(socket->IsNonBlocking() && socket->HasMessage)
    {

    }
}

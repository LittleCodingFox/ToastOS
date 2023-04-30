#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "abi-bits/socket.h"
#include "sys/un.h"

int64_t SyscallSendMsg(InterruptStack *stack)
{
    int sockfd = stack->rsi;
    struct msghdr *hdr = (struct msghdr *)stack->rdx;
    int flags = stack->rcx;

    (void)flags;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: sendmsg sockfd %d hdr %p flags %x", sockfd, hdr, flags);
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

    if(socket->socket->Closed() || socket->socket->ConnectionRefused() || socket->socket->GetPeer() == NULL)
    {
        return -ECONNREFUSED;
    }

    socket->socket->GetPeer()->EnqueueMessage(hdr->msg_iov->iov_base, hdr->msg_iov->iov_len);

    return hdr->msg_iov->iov_len;
}

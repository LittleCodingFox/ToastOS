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
        DEBUG_OUT("BADF", 0);
        return -EBADF;
    }

    if(fd->type != ProcessFDType::Socket)
    {
        DEBUG_OUT("NOTSOCK", 0);
        return -ENOTSOCK;
    }

    auto socket = (ProcessFDSocket *)fd->impl;
    size_t length = 0;

    vector<uint8_t> message;

    if((flags & MSG_PEEK) == MSG_PEEK)
    {
        DEBUG_OUT("PEEK", 0);

        if(socket->HasMessage())
        {
            message = socket->GetMessage(true);
        }
    }
    else
    {
        if(socket->IsNonBlocking() && socket->HasMessage() == false)
        {
            DEBUG_OUT("EWOULDBLOCK", 0);
            return -EWOULDBLOCK;
        }

        while(socket->HasMessage() == false && socket->Connected())
        {
            ProcessYield();
        }

        if(socket->Connected())
        {
            message = socket->GetMessage(false);
        }
    }

    DEBUG_OUT("Message size: %i; socket connected: %s", message.size(), socket->Connected() ? "YES" : "NO");

    length = message.size() < hdr->msg_iov->iov_len ? message.size() : hdr->msg_iov->iov_len;

    if(length > 0)
    {
        memcpy(hdr->msg_iov->iov_base, message.data(), length);

        return length;
    }

    return 0;
}

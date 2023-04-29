#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "abi-bits/socket.h"
#include "sys/un.h"

int64_t SyscallAccept(InterruptStack *stack)
{
    int sockfd = stack->rsi;
    struct sockaddr *address = (struct sockaddr *)stack->rdx;
    socklen_t addressLength = stack->rcx;

    (void)address;
    (void)addressLength;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: accept sockfd %d address %p addressLength %i", sockfd, address, addressLength);
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

    for(;;)
    {
        auto peer = socket->PendingPeer();

        if(peer != NULL)
        {
            peer = socket->AddPeer(peer->fd);

            ((ProcessFDSocket *)peer->fd->impl)->AddPeer(fd);

            return process->info->AddFD(ProcessFDType::Socket, peer->fd);
        }

        if(socket->IsNonBlocking())
        {
            return -EWOULDBLOCK;
        }

        ProcessYield();
    }
}

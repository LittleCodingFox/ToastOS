#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "net/UnixSocket.hpp"
#include "net/SocketManager.hpp"

int64_t SyscallListen(InterruptStack *stack)
{
    int sockfd = stack->rsi;
    int backlog = stack->rdx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: listen sockfd %d backlog %i", sockfd, backlog);
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

    if(socket->socket == NULL)
    {
        return -EBADF;
    }

    socket->socket->listenLimit = backlog;

    return 0;
}

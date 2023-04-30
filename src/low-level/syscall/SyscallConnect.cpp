#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "net/UnixSocket.hpp"
#include "net/SocketManager.hpp"

struct UnixConnectUserdata
{
    string path;
    UnixSocket *socket;
};

int64_t SyscallConnect(InterruptStack *stack)
{
    int sockfd = stack->rsi;
    const struct sockaddr *address = (const struct sockaddr *)stack->rdx;
    socklen_t addressLength = stack->rcx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: connect sockfd %d address %p addressLength %i", sockfd, address, addressLength);
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

    if(socket->socket->domain == PF_UNIX)
    {
        if(addressLength != sizeof(sockaddr_un))
        {
            return -EINVAL;
        }

        auto unixSocket = (const struct sockaddr_un *)address;

        auto file = vfs->FindVirtualFile(unixSocket->sun_path);

        if(file == NULL)
        {
            return -EADDRNOTAVAIL;
        }

        if(file->type != FILE_HANDLE_SOCKET)
        {
            return -EHOSTUNREACH;
        }

        UnixConnectUserdata userdata;

        userdata.path = file->path;
        userdata.socket = NULL;

        socketManager->IterateSockets(PF_UNIX, [](ISocket *socket, void *userdata) {

            auto u = (UnixConnectUserdata *)userdata;
            auto unixSocket = (UnixSocket *)socket;

            if(unixSocket->path == u->path)
            {
                u->socket = unixSocket;
            }
        }, &userdata);

        if(userdata.socket == NULL)
        {
            return -ECONNREFUSED;
        }

        userdata.socket->AddPendingPeer(socket->socket);

        while(socket->Connected() == false && socket->ConnectionRefused() == false)
        {
            ProcessYield();
        }

        if(socket->ConnectionRefused())
        {
            return -ECONNREFUSED;
        }

        return 0;
    }

    return -ENOTSUP;
}

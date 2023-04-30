#include "process/Process.hpp"
#include "UnixSocket.hpp"

void UnixSocket::Close()
{
    closed = true;
}

void UnixSocket::RefuseConnection()
{
}

bool UnixSocket::ConnectionRefused()
{
    return false;
}

bool UnixSocket::IsNonBlocking()
{
    return false;
}

bool UnixSocket::IsConnected()
{
    return peer.isValid && peer.peer != NULL;
}

bool UnixSocket::Closed()
{
    return closed;
}

void UnixSocket::Connect()
{

}

void UnixSocket::Accept()
{

}

int32_t UnixSocket::Bind(const struct sockaddr *addr, socklen_t length, Process *process)
{
    if(length != sizeof(sockaddr_un))
    {
        return -EINVAL;
    }

    const sockaddr_un *sockdata = (const sockaddr_un *)addr;

    if(sockdata == NULL || strlen(sockdata->sun_path) == 0)
    {
        return -EINVAL;
    }

    int error;

    auto handle = vfs->OpenFile(sockdata->sun_path, O_RDONLY, process, &error);

    if(handle != INVALID_FILE_HANDLE)
    {
        vfs->CloseFile(handle);

        return -EINVAL;
    }

    VirtualFile file;

    memset(&file, 0, sizeof(VirtualFile));

    path = sockdata->sun_path;

    if(path[0] != '/')
    {
        path = process->cwd + "/" + path;
    }

    file.path = path;
    file.type = FILE_HANDLE_SOCKET;

    vfs->AddVirtualFile(file);

    return 0;
}

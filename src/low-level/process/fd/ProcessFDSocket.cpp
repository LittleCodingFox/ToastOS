#include "../Process.hpp"
#include "fcntl.h"
#include "filesystems/VFS.hpp"
#include "errno.h"

ProcessFDSocket::ProcessFDSocket(ISocket *socket) : socket(socket) {}

bool ProcessFDSocket::IsNonBlocking()
{
    if(socket == NULL)
    {
        return true;
    }

    return socket->IsNonBlocking();
}

void ProcessFDSocket::RefuseConnection()
{
    if(socket == NULL)
    {
        return;
    }

    socket->RefuseConnection();
}

bool ProcessFDSocket::ConnectionRefused()
{
    if(socket == NULL)
    {
        return true;
    }

    return socket->ConnectionRefused();
}

bool ProcessFDSocket::Connected()
{
    if(socket == NULL)
    {
        return false;
    }

    return socket->IsConnected();
}

bool ProcessFDSocket::HasMessage()
{
    if(socket == NULL)
    {
        return false;
    }

    return socket->HasMessage();
}

void ProcessFDSocket::EnqueueMessage(const void *buffer, size_t length)
{
    if(socket == NULL)
    {
        return;
    }

    socket->EnqueueMessage(buffer, length);
}

vector<uint8_t> ProcessFDSocket::GetMessage(bool peek)
{
    if(socket == NULL)
    {
        return vector<uint8_t>();
    }

    return socket->GetMessage(peek);
}

void ProcessFDSocket::Close()
{
    if(socket == NULL)
    {
        return;
    }

    socket->Close();

    socket = NULL;
}

uint64_t ProcessFDSocket::Read(void *buffer, uint64_t length, int *error)
{
    if(socket == NULL)
    {
        return 0;
    }

    if(socket->IsNonBlocking() && socket->HasMessage() == false)
    {
        *error = EWOULDBLOCK;

        return 0;
    }

    while(socket != NULL && socket->HasMessage() == false && socket->Closed() == false)
    {
        ProcessYield();

        if(socket != NULL && socket->HasMessage())
        {
            auto message = socket->GetMessage(false);

            length = message.size() < length ? message.size() : length;

            if(length > 0)
            {
                memcpy(buffer, message.data(), length);
            }

            return length;
        }
    }

    return 0;
}

uint64_t ProcessFDSocket::Write(const void *buffer, uint64_t length, int *error)
{
    if(socket == NULL)
    {
        return 0;
    }

    socket->EnqueueMessage(buffer, length);

    return length;
}

int64_t ProcessFDSocket::Seek(uint64_t offset, int whence, int *error)
{
    *error = ESPIPE;

    return 0;
}

dirent *ProcessFDSocket::ReadEntries()
{
    return NULL;
}

struct stat ProcessFDSocket::Stat(int *error)
{
    return {};
}

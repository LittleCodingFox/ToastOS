#include "../Process.hpp"
#include "fcntl.h"
#include "filesystems/VFS.hpp"
#include "errno.h"

extern "C" void ProcessYield();

bool ProcessFDSocket::Peer::Closed()
{
    return isValid == false ||
        fd == NULL ||
        fd->type != ProcessFDType::Socket ||
        fd->isValid == false ||
        ((ProcessFDSocket *)fd->impl)->closed;
}

ProcessFDSocket::ProcessFDSocket() : closed(false), connectionRefused(false), nonBlocking(false), domain(0), type(0), protocol(0), port(0) {}

ProcessFDSocket::ProcessFDSocket(uint32_t domain, uint32_t type, uint32_t protocol, uint16_t port) :
    closed(false), connectionRefused(false), nonBlocking(false), domain(domain), type(type), protocol(protocol), port(port) {}

bool ProcessFDSocket::IsNonBlocking()
{
    ScopedLock lock(socketLock);

    return nonBlocking;
}

ProcessFDSocket::Peer *ProcessFDSocket::AddPeer(ProcessFD *fd)
{
    ScopedLock lock(socketLock);

    for(auto &pending : pendingPeers)
    {
        if(pending->isValid && pending->fd == fd)
        {
            pending->isValid = false;

            break;
        }
    }

    auto peer = new Peer();

    peer->isValid = true;
    peer->fd = fd;

    peers.push_back(peer);

    return peer;
}

void ProcessFDSocket::AddPendingPeer(ProcessFD *fd)
{
    if(fd == NULL || fd->isValid == false || fd->type != ProcessFDType::Socket)
    {
        return;
    }

    ScopedLock lock(socketLock);

    for(auto &peer : peers)
    {
        if(peer->isValid && peer->fd == fd)
        {
            return;
        }
    }

    for(auto &pending : pendingPeers)
    {
        if(pending->isValid && pending->fd == fd)
        {
            return;
        }
    }

    for(auto &pending : pendingPeers)
    {
        if(pending->isValid == false)
        {
            pending->isValid = true;
            pending->fd = fd;

            return;
        }
    }

    auto peer = new Peer();

    peer->isValid = true;
    peer->fd = fd;

    pendingPeers.push_back(peer);
}

ProcessFDSocket::Peer *ProcessFDSocket::PendingPeer()
{
    ScopedLock lock(socketLock);

    for(auto &pending : pendingPeers)
    {
        if(pending != NULL && pending->isValid && pending->fd->isValid)
        {
            return pending;
        }
    }

    return NULL;
}

void ProcessFDSocket::RefuseConnection()
{
    ScopedLock lock(socketLock);

    connectionRefused = true;
}

bool ProcessFDSocket::ConnectionRefused()
{
    ScopedLock lock(socketLock);

    return connectionRefused;
}

bool ProcessFDSocket::Connected()
{
    ScopedLock lock(socketLock);

    for(auto &peer : peers)
    {
        if(peer->isValid && peer->Closed() == false)
        {
            return true;
        }
    }

    return false;
}

ProcessFDSocket::Peer *ProcessFDSocket::FindPeer(ProcessFDSocket *socket)
{
    ScopedLock lock(socketLock);

    for(auto &peer : peers)
    {
        if(peer->isValid && peer->fd->isValid && peer->fd->impl == socket)
        {
            return peer;
        }
    }

    return NULL;
}

bool ProcessFDSocket::HasMessage(Peer *peer)
{
    ScopedLock lock(socketLock);

    if(peer == NULL || peer->isValid == false || peer->Closed())
    {
        return false;
    }

    for(auto &message : messages)
    {
        if(message->isValid && message->peer == peer)
        {
            return true;
        }
    }

    return false;
}

void ProcessFDSocket::EnqueueMessage(const void *buffer, uint64_t length, Peer *peer)
{
    ScopedLock lock(socketLock);

    if(peer == NULL || peer->isValid == false || peer->Closed())
    {
        return;
    }

    for(auto &message : messages)
    {
        if(message->isValid == false)
        {
            message->isValid = true;
            message->peer = peer;

            message->buffer.resize(length);

            memcpy(message->buffer.data(), buffer, length);

            return;
        }
    }

    auto message = new Message();

    message->isValid = true;
    message->peer = peer;
    message->buffer.resize(length);

    memcpy(message->buffer.data(), buffer, length);

    messages.push_back(message);
}

vector<uint8_t> ProcessFDSocket::GetMessage(Peer *peer)
{
    ScopedLock lock(socketLock);

    if(peer == NULL || peer->isValid == false || peer->Closed())
    {
        return vector<uint8_t>();
    }

    for(auto &message : messages)
    {
        if(message->isValid && message->peer == peer)
        {
            message->isValid = false;

            auto outVector = message->buffer;

            message->buffer.clear();

            return outVector;
        }
    }

    return vector<uint8_t>();
}

vector<uint8_t> ProcessFDSocket::PeekMessage(Peer *peer)
{
    ScopedLock lock(socketLock);

    if(peer == NULL || peer->isValid == false || peer->Closed())
    {
        return vector<uint8_t>();
    }

    for(auto &message : messages)
    {
        if(message->isValid && message->peer == peer)
        {
            return message->buffer;
        }
    }

    return vector<uint8_t>();
}

void ProcessFDSocket::Close()
{
    ScopedLock lock(socketLock);

    closed = true;

    for(auto &peer : peers)
    {
        if(peer->isValid && peer->fd->isValid && peer->fd->type == ProcessFDType::Socket)
        {
            ((ProcessFDSocket *)peer->fd->impl)->closed = true;
        }
    }

    for(auto &message : messages)
    {
        delete message;
    }

    messages.clear();
}

uint64_t ProcessFDSocket::Read(void *buffer, uint64_t length, int *error)
{
    socketLock.Lock();

    if(peers.size() != 1)
    {
        socketLock.Unlock();

        *error = ENOTCONN;

        return 0;
    }

    auto peer = peers[0];

    if(peer == NULL || peer->isValid == false || closed || peer->Closed())
    {
        socketLock.Unlock();

        *error = ENOTCONN;

        return 0;
    }

    if(nonBlocking && HasMessage(peer) == false)
    {
        socketLock.Unlock();

        *error = EWOULDBLOCK;

        return 0;
    }

    socketLock.Unlock();

    for(;;)
    {
        socketLock.Lock();

        if(peer->Closed())
        {
            socketLock.Unlock();

            return 0;
        }

        if(HasMessage(peer) == false)
        {
            socketLock.Unlock();

            ProcessYield();

            continue;
        }

        break;            
    }

    socketLock.Unlock();

    auto message = GetMessage(peer);

    if(message.size() < length)
    {
        length = message.size();
    }

    memcpy(buffer, message.data(), length);

    return length;
}

uint64_t ProcessFDSocket::Write(const void *buffer, uint64_t length, int *error)
{
    ScopedLock lock(socketLock);

    if(peers.size() != 1)
    {
        *error = ENOTCONN;

        return 0;
    }

    auto peer = peers[0];

    if(peer == NULL || peer->isValid == false || closed || peer->Closed())
    {
        *error = ENOTCONN;

        return 0;
    }

    auto otherSocket = (ProcessFDSocket *)peer->fd->impl;

    auto otherPeer = otherSocket->FindPeer(this);

    if(otherPeer == NULL || otherPeer->Closed())
    {
        *error = ENOTCONN;

        return 0;
    }

    otherSocket->EnqueueMessage(buffer, length, otherPeer);

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

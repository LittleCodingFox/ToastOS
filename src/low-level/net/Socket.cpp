#include "Socket.hpp"
#include "SocketManager.hpp"

ISocket::ISocket()
{
    socketManager->AddSocket(this);
}

ISocket::~ISocket()
{
    socketManager->RemoveSocket(this);
}

ISocket *ISocket::PendingPeer()
{
    ScopedLock lock(socketLock);

    for(auto &peer : pendingPeers)
    {
        if(peer->isValid)
        {
            peer->isValid = false;

            return peer->peer;
        }
    }

    return NULL;
}

void ISocket::AddPendingPeer(ISocket *in)
{
    ScopedLock lock(socketLock);

    for(auto &peer : pendingPeers)
    {
        if(peer->isValid == false)
        {
            peer->isValid = true;
            peer->peer = in;

            return;
        }
    }

    auto peer = new SocketPeer();

    peer->isValid = true;
    peer->peer = in;

    pendingPeers.push_back(peer);
}

void ISocket::SetPeer(ISocket *in)
{
    ScopedLock lock(socketLock);

    peer.isValid = true;
    peer.peer = in;
}

bool ISocket::HasMessage()
{
    ScopedLock lock(socketLock);

    for(auto &message : messages)
    {
        if(message->isValid)
        {
            return true;
        }
    }

    return false;
}

vector<uint8_t> ISocket::GetMessage(bool peek)
{
    ScopedLock lock(socketLock);

    for(auto &message : messages)
    {
        if(message->isValid)
        {
            if(peek == false)
            {
                message->isValid = false;
            }

            return message->buffer;
        }
    }

    return vector<uint8_t>();
}

void ISocket::EnqueueMessage(const void *buffer, size_t length)
{
    ScopedLock lock(socketLock);

    SocketMessage *target = NULL;

    for(auto &message : messages)
    {
        if(message->isValid == false)
        {
            target = message;

            break;
        }
    }

    bool shouldAdd = target == NULL;

    if(target == NULL)
    {
        target = new SocketMessage();
    }

    target->isValid = true;
    target->buffer.resize(length);

    memcpy(target->buffer.data(), buffer, length);

    if(shouldAdd)
    {
        messages.push_back(target);
    }
}

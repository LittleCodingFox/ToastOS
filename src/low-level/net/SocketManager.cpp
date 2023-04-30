#include "SocketManager.hpp"

box<SocketManager> socketManager;

void SocketManager::AddSocket(ISocket *socket)
{
    ScopedLock lock(socketLock);

    for(auto &info : sockets)
    {
        if(info->isValid == false)
        {
            info->isValid = true;
            info->socket = socket;

            return;
        }
    }

    auto info = new SocketInfo();

    info->isValid = true;
    info->socket = socket;

    sockets.push_back(info);
}

void SocketManager::RemoveSocket(ISocket *socket)
{
    ScopedLock lock(socketLock);

    for(auto &info : sockets)
    {
        if(info->isValid && info->socket == socket)
        {
            info->isValid = false;
        }
    }
}

void SocketManager::IterateSockets(void (*callback)(ISocket *socket))
{
    ScopedLock lock(socketLock);

    for(auto &info : sockets)
    {
        if(info->isValid)
        {
            callback(info->socket);
        }
    }
}

void SocketManager::IterateSockets(uint32_t domain, void (*callback)(ISocket *socket))
{
    ScopedLock lock(socketLock);

    for(auto &info : sockets)
    {
        if(info->isValid && info->socket->domain == domain)
        {
            callback(info->socket);
        }
    }
}

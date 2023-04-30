#pragma once

#include "Socket.hpp"

class SocketManager
{
private:
    struct SocketInfo
    {
        bool isValid;
        ISocket *socket;

        SocketInfo() : isValid(false), socket(NULL) {}
    };

    vector<SocketInfo *> sockets;

    AtomicLock socketLock;
public:
    void AddSocket(ISocket *socket);
    void RemoveSocket(ISocket *socket);

    void IterateSockets(void (*callback)(ISocket *socket, void *userdata), void *userdata);
    void IterateSockets(uint32_t domain, void (*callback)(ISocket *socket, void *userdata), void *userdata);
};

extern box<SocketManager> socketManager;

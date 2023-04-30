#pragma once

#include "filesystems/VFS.hpp"
#include "fcntl.h"
#include "errno.h"
#include "abi-bits/socket.h"
#include "sys/un.h"

class ISocket;

struct SocketMessage
{
    vector<uint8_t> buffer;
    bool isValid;

    SocketMessage() : isValid(false) {}
};

struct SocketPeer
{
    ISocket *peer;
    bool isValid;

    SocketPeer() : isValid(false) {}

    bool Closed();
};

class ISocket
{
protected:
    AtomicLock socketLock;

    vector<SocketMessage *> messages;
    SocketPeer peer;
public:
    uint32_t domain;
    uint32_t type;
    uint32_t protocol;
    uint16_t port;

    ISocket() : domain(0), type(0), protocol(0), port(0) {}

    virtual bool HasMessage();
    virtual vector<uint8_t> GetMessage(bool peek);
    virtual void Close() = 0;
    virtual void EnqueueMessage(const void *buffer, size_t length);
    virtual void RefuseConnection() = 0;
    virtual bool ConnectionRefused() = 0;
    virtual bool IsNonBlocking() = 0;
    virtual bool IsConnected() = 0;
    virtual bool Closed() = 0;
    virtual void Connect() = 0;
    virtual void Accept() = 0;
    virtual int32_t Bind(const struct sockaddr *addr, socklen_t length, Process *process) = 0;
};

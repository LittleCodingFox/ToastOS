#pragma once

#include "Socket.hpp"

class UnixSocket : public ISocket
{
private:
    bool closed;
public:
    string path;

    UnixSocket(uint32_t type, uint32_t protocol) : closed(false)
    {
        domain = AF_UNIX;
        this->type = type;
        this->protocol = protocol;
    }

    virtual void Close() override;
    virtual void RefuseConnection() override;
    virtual bool ConnectionRefused() override;
    virtual bool IsNonBlocking() override;
    virtual bool IsConnected() override;
    virtual bool Closed() override;
    virtual void Connect() override;
    virtual void Accept() override;
    virtual int32_t Bind(const struct sockaddr *addr, socklen_t length, Process *process) override;
};

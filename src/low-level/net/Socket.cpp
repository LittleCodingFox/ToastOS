#include "Socket.hpp"

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

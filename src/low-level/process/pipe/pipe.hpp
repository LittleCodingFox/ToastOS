#pragma once

#include "kernel.h"
#include "CircularBuffer.hpp"

#define PIPE_MESSAGE_SIZE 1024

struct PipeMessage
{
    uint8_t *data;
    size_t size;
};

class Pipe
{
private:
    CircularBuffer<PipeMessage> messages;
    bool closedWriter;
    bool closedReader;
public:
    AtomicLock lock;

    Pipe() : messages(PIPE_MESSAGE_SIZE), closedWriter(false), closedReader(false) {}

    PipeMessage *Get()
    {
        auto message = messages.Get();

        return message;
    }

    void Put(PipeMessage message)
    {
        messages.Put(message);
    }

    void CloseWriter()
    {
        closedWriter = true;
    }

    void CloseReader()
    {
        closedReader = true;
    }

    inline bool ClosedWriter() const
    {
        return closedWriter;
    }

    inline bool ClosedReader() const
    {
        return closedReader;
    }

    inline size_t Capacity() const
    {
        return messages.Capacity();
    }

    inline bool Full() const
    {
        return messages.Full();
    }

    inline bool Empty() const
    {
        return messages.Empty();
    }
};

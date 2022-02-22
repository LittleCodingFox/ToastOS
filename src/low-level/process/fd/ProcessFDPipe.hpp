#include "../Process.hpp"
#include "process/pipe/pipe.hpp"
#include "fcntl.h"
#include "errno.h"

class ProcessFDPipe : public IProcessFD
{
public:
    Pipe *pipe;
    bool isRead;

    explicit ProcessFDPipe(Pipe *pipe, bool isRead) : pipe(pipe), isRead(isRead) {}

    virtual void Close() override
    {
        refCount--;

        if(refCount == 0)
        {
            if(isRead)
            {
                pipe->CloseReader();
            }
            else
            {
                pipe->CloseWriter();
            }
        }
    }

    virtual uint64_t Read(void *buffer, uint64_t length, int *error) override
    {
        if(isRead)
        {
            pipe->lock.Lock();

            if((pipe->Empty() && pipe->ClosedWriter()) || pipe->ClosedReader())
            {
                pipe->lock.Unlock();

                return 0;
            }

            auto message = pipe->Get();

            while(message == NULL || message->data == NULL || message->size == 0)
            {
                pipe->lock.Unlock();

                if((pipe->Empty() && pipe->ClosedWriter()) || pipe->ClosedReader())
                {
                    return 0;
                }

                ProcessYield();

                pipe->lock.Lock();

                message = pipe->Get();
            }

            size_t size = length > message->size ? message->size : length;

            memcpy(buffer, message->data, size);

            delete [] message->data;

            message->data = NULL;
            message->size = 0;

            pipe->lock.Unlock();

            return size;
        }

        return 0;
    }

    virtual uint64_t Write(const void *buffer, uint64_t length, int *error) override
    {
        if(isRead)
        {
            return 0;
        }

        pipe->lock.Lock();

        if(pipe->ClosedWriter())
        {
            pipe->lock.Unlock();

            return 0;
        }

        PipeMessage message;
        message.size = length;
        message.data = new uint8_t[length];

        memcpy(message.data, buffer, length);

        pipe->Put(message);

        pipe->lock.Unlock();

        return length;
    }

    virtual int64_t Seek(uint64_t offset, int whence, int *error) override
    {
        *error = ESPIPE;

        return 0;
    }

    virtual dirent *ReadEntries() override
    {
        return NULL;
    }

    virtual struct stat Stat(int *error) override
    {
        return {};
    }
};
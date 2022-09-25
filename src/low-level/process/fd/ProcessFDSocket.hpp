#include "../Process.hpp"
#include "filesystems/VFS.hpp"
#include "fcntl.h"
#include "errno.h"

class ProcessFDSocket : public IProcessFD
{
private:
    vector<uint8_t> buffer;
    uint32_t bufferIndex;
    AtomicLock socketLock;
public:
    uint32_t domain;
    uint32_t type;
    uint32_t protocol;
    uint16_t port;

    ProcessFDSocket() : bufferIndex(0), domain(0), type(0), protocol(0), port(0) {}

    ProcessFDSocket(uint32_t domain, uint32_t type, uint32_t protocol, uint16_t port = 0) :
        bufferIndex(0), domain(domain), type(type), protocol(protocol), port(port) {}

    virtual void Close() override {}

    virtual uint64_t Read(void *buffer, uint64_t length, int *error) override
    {
        for(;;)
        {
            socketLock.Lock();

            if(bufferIndex >= this->buffer.size())
            {
                socketLock.Unlock();

                ProcessYield();

                continue;
            }

            socketLock.Unlock();

            break;            
        }

        socketLock.Lock();

        if(length > this->buffer.size())
        {
            length = this->buffer.size();
        }

        memcpy(buffer, this->buffer.data() + bufferIndex, length);

        if(length < this->buffer.size())
        {
            bufferIndex += length;
        }
        else
        {
            bufferIndex = 0;
            this->buffer.resize(0);
        }

        socketLock.Unlock();

        return length;
    }

    virtual uint64_t Write(const void *buffer, uint64_t length, int *error) override
    {
        ScopedLock lock(socketLock);

        uint64_t current = this->buffer.size();

        this->buffer.resize(current + length);
        memcpy(this->buffer.data() + current, buffer, length);

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

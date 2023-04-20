#include "../Process.hpp"
#include "fcntl.h"
#include "filesystems/VFS.hpp"
#include "errno.h"

extern "C" void ProcessYield();

ProcessFDSocket::ProcessFDSocket() : bufferIndex(0), domain(0), type(0), protocol(0), port(0) {}

ProcessFDSocket::ProcessFDSocket(uint32_t domain, uint32_t type, uint32_t protocol, uint16_t port) :
    bufferIndex(0), domain(domain), type(type), protocol(protocol), port(port) {}

void ProcessFDSocket::Close() {}

uint64_t ProcessFDSocket::Read(void *buffer, uint64_t length, int *error)
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

uint64_t ProcessFDSocket::Write(const void *buffer, uint64_t length, int *error)
{
    ScopedLock lock(socketLock);

    uint64_t current = this->buffer.size();

    this->buffer.resize(current + length);
    memcpy(this->buffer.data() + current, buffer, length);

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

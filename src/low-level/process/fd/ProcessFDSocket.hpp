#include "../Process.hpp"
#include "filesystems/VFS.hpp"
#include "fcntl.h"
#include "errno.h"

class ProcessFDSocket : public IProcessFD
{
public:
    uint32_t domain;
    uint32_t type;
    uint32_t protocol;
    uint16_t port;

    ProcessFDSocket() : domain(0), type(0), protocol(0), port(0) {}

    ProcessFDSocket(uint32_t domain, uint32_t type, uint32_t protocol, uint16_t port = 0) :
        domain(domain), type(type), protocol(protocol), port(port) {}

    virtual void Close() override {}

    virtual uint64_t Read(void *buffer, uint64_t length, int *error) override
    {
        return 0;
    }

    virtual uint64_t Write(const void *buffer, uint64_t length, int *error) override
    {
        return 0;
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

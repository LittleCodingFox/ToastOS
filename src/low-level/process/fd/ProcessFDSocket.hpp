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

    ProcessFDSocket();

    ProcessFDSocket(uint32_t domain, uint32_t type, uint32_t protocol, uint16_t port = 0);

    virtual void Close() override;

    virtual uint64_t Read(void *buffer, uint64_t length, int *error) override;

    virtual uint64_t Write(const void *buffer, uint64_t length, int *error) override;

    virtual int64_t Seek(uint64_t offset, int whence, int *error) override;

    virtual dirent *ReadEntries() override;

    virtual struct stat Stat(int *error) override;
};

#include "../Process.hpp"
#include "filesystems/VFS.hpp"
#include "fcntl.h"
#include "errno.h"
#include "net/Socket.hpp"

class ProcessFDSocket : public IProcessFD
{
public:
    ISocket *socket;

    ProcessFDSocket(ISocket *socket);

    bool IsNonBlocking();

    bool Connected();

    bool ConnectionRefused();

    void RefuseConnection();

    void EnqueueMessage(const void *buffer, size_t length);

    bool HasMessage();

    vector<uint8_t> GetMessage(bool peek);

    virtual void Close() override;

    virtual uint64_t Read(void *buffer, uint64_t length, int *error) override;

    virtual uint64_t Write(const void *buffer, uint64_t length, int *error) override;

    virtual int64_t Seek(uint64_t offset, int whence, int *error) override;

    virtual dirent *ReadEntries() override;

    virtual struct stat Stat(int *error) override;
};

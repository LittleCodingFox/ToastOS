#include "../Process.hpp"
#include "filesystems/VFS.hpp"
#include "fcntl.h"
#include "errno.h"

class ProcessFDStdout : public IProcessFD
{
public:
    virtual uint64_t Read(void *buffer, uint64_t length) override
    {
        return 0;
    }

    virtual uint64_t Write(const void *buffer, uint64_t length) override
    {
        printf("%.*s", length, buffer);

        return length;
    }

    virtual int64_t Seek(uint64_t offset, int whence) override
    {
        return -ESPIPE;
    }

    virtual dirent *ReadEntries() override
    {
        return NULL;
    }

    virtual stat Stat() override
    {
        return {};
    }
};
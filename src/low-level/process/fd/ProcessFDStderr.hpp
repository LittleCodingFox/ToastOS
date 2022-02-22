#include "../Process.hpp"
#include "filesystems/VFS.hpp"
#include "fcntl.h"
#include "errno.h"

class ProcessFDStderr : public IProcessFD
{
public:
    virtual void Close() override
    {
        refCount = 0;
    }

    virtual uint64_t Read(void *buffer, uint64_t length, int *error) override
    {
        return 0;
    }

    virtual uint64_t Write(const void *buffer, uint64_t length, int *error) override
    {
        printf("%.*s", length, buffer);

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
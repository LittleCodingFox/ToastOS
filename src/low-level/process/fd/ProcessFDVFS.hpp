#include "../Process.hpp"
#include "filesystems/VFS.hpp"
#include "fcntl.h"
#include "errno.h"

class ProcessFDVFS : public IProcessFD
{
public:
    FILE_HANDLE handle;

    ProcessFDVFS(FILE_HANDLE handle) : handle(handle) {}

    virtual void Close() override
    {
        refCount--;

        if(refCount == 0)
        {
            if(vfs.valid())
            {
                vfs->CloseFile(handle);
            }
        }
    }

    virtual uint64_t Read(void *buffer, uint64_t length, int *error) override
    {
        if(vfs.valid())
        {
            return vfs->ReadFile(handle, buffer, length, error);
        }

        return 0;
    }

    virtual uint64_t Write(const void *buffer, uint64_t length, int *error) override
    {
        if(vfs.valid())
        {
            return vfs->WriteFile(handle, buffer, length, error);
        }

        return 0;
    }

    virtual int64_t Seek(uint64_t offset, int whence, int *error) override
    {
        if(vfs.valid())
        {
            switch(whence)
            {
                case SEEK_SET:

                    return vfs->SeekFile(handle, offset, error);

                case SEEK_CUR:

                    if(vfs->FileOffset(handle) + offset > vfs->FileLength(handle))
                    {
                        *error = EINVAL;

                        return 0;
                    }

                    return vfs->SeekFile(handle, vfs->FileOffset(handle) + offset, error);

                case SEEK_END:

                    return vfs->SeekFileEnd(handle, error);
            }
        }

        return 0;
    }

    virtual dirent *ReadEntries() override
    {
        if(vfs.valid())
        {
            return vfs->ReadEntries(handle);
        }

        return NULL;
    }

    virtual struct stat Stat(int *error) override
    {
        if(vfs.valid())
        {
            return vfs->Stat(handle, error);
        }

        return {};
    }
};
#include "../Process.hpp"
#include "filesystems/VFS.hpp"
#include "fcntl.h"
#include "errno.h"

class ProcessFDVFS : public IProcessFD
{
public:
    FILE_HANDLE handle;

    ProcessFDVFS(FILE_HANDLE handle) : handle(handle) {}
    ~ProcessFDVFS()
    {
        if(vfs.valid())
        {
            vfs->CloseFile(handle);
        }
    }

    virtual uint64_t Read(void *buffer, uint64_t length) override
    {
        if(vfs.valid())
        {
            return vfs->ReadFile(handle, buffer, length);
        }

        return 0;
    }

    virtual uint64_t Write(const void *buffer, uint64_t length) override
    {
        if(vfs.valid())
        {
            return vfs->WriteFile(handle, buffer, length);
        }

        return 0;
    }

    virtual int64_t Seek(uint64_t offset, int whence) override
    {
        if(vfs.valid())
        {
            switch(whence)
            {
                case SEEK_SET:

                    return vfs->SeekFile(handle, offset);

                case SEEK_CUR:

                    if(vfs->FileOffset(handle) + offset > vfs->FileLength(handle))
                    {
                        return -EINVAL;
                    }

                    return vfs->SeekFile(handle, vfs->FileOffset(handle) + offset);

                case SEEK_END:

                    return vfs->SeekFileEnd(handle);
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

    virtual stat Stat() override
    {
        if(vfs.valid())
        {
            return vfs->Stat(handle);
        }

        return {};
    }
};
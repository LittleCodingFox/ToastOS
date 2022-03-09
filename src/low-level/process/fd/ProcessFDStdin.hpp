#include "../Process.hpp"

class ProcessFDStdin : public IProcessFD
{
public:
    virtual void Close() override;

    virtual uint64_t Read(void *buffer, uint64_t length, int *error) override;

    virtual uint64_t Write(const void *buffer, uint64_t length, int *error) override;

    virtual int64_t Seek(uint64_t offset, int whence, int *error) override;

    virtual dirent *ReadEntries() override;

    virtual struct stat Stat(int *error) override;
};
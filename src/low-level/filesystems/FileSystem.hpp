#pragma once
#include <stdint.h>
#include "lock.hpp"
#include "devicemanager/GenericIODevice.hpp"
#include "gpt/GPT.hpp"

namespace FileSystem
{
    class File
    {
    public:
        virtual const char *Path() = 0;
        virtual uint64_t Sie() = 0;

    protected:
        uint64_t fileID;
        uint64_t address;
        const char *path;
    };

    enum FileHandleType
    {
        FILE_HANDLE_UNKNOWN,
        FILE_HANDLE_DIRECTORY,
        FILE_HANDLE_FILE,
    };

    typedef uint64_t FileSystemHandle;

    class FileSystem
    {
    protected:
        GPT::Partition *partition;
    public:
        Threading::AtomicLock lock;

        FileSystem(GPT::Partition *partition);

        virtual void Initialize(uint64_t sector, uint64_t sectorCount) = 0;

        virtual const char *VolumeName() = 0;

        virtual FileSystemHandle GetFileHandle(const char *path) = 0;

        virtual void DisposeFileHandle(FileSystemHandle handle) = 0;

        virtual FileHandleType FileHandleType(FileSystemHandle handle) = 0;

        virtual uint64_t FileLength(FileSystemHandle handle) = 0;

        virtual uint64_t ReadFile(FileSystemHandle handle, void *buffer, uint64_t cursor, uint64_t size) = 0;

        virtual uint64_t WriteFile(FileSystemHandle handle, const void *buffer, uint64_t cursor, uint64_t size) = 0;

        virtual bool Exists(const char *path) = 0;

        virtual void DebugListDirectories() = 0;
    };
}

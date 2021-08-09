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

    class FileSystem
    {
    protected:
        GPT::Partition *partition;
    public:
        Threading::AtomicLock lock;

        FileSystem(GPT::Partition *partition);

        virtual void Initialize(uint64_t sector, uint64_t sectorCount) = 0;

        virtual const char *VolumeName() = 0;

        virtual uint64_t FileLength(const char *path) = 0;

        virtual uint64_t ReadFile(const char *path, void *buffer, uint64_t cursor, uint64_t size) = 0;

        virtual uint64_t WriteFile(const char *path, const void *buffer, uint64_t cursor, uint64_t size) = 0;

        virtual bool Exists(const char *path) = 0;

        virtual void DebugListDirectories() = 0;
    };
}

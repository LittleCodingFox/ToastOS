#pragma once

#include <stddef.h>
#include <stdint.h>
#include <frg_allocator.hpp>
#include <frg/vector.hpp>
#include "devicemanager/GenericIODevice.hpp"
#include "filesystems/FileSystem.hpp"

namespace FileSystem
{
    namespace tarfs
    {
        enum TarFileType
        {
            TAR_FILE = '0',
            TAR_LINK = '1',
            TAR_SYMLINK = '2',
            TAR_CHARDEVICE = '3',
            TAR_BLOCKDEVICE = '4',
            TAR_DIRECTORY = '5',
            TAR_FIFO = '6'
        };

        struct __attribute__((packed)) TarHeader
        {
            char name[100];
            char mode[8];
            char uid[8];
            char gid[8];
            char size[12];
            char mtime[12];
            char checksum[8];
            char type;
            char link[100];
            char ustar[6];
            char version[2];
            char uname[32];
            char gname[32];
            char deviceMajorNumber[8];
            char deviceMinorNumber[8];
            char prefix[155];
        };

        static_assert(sizeof(TarHeader) == 500);

        class TarFS : public FileSystem
        {
        private:
            struct FileHandleData
            {
                uint64_t ID;
                TarHeader *header;
                uint64_t length;
            };

            uint64_t fileHandleCounter;
            uint8_t *data;

            frg::vector<FileHandleData, frg_allocator> fileHandles;
            frg::vector<TarHeader *, frg_allocator> headers;

            uint64_t GetHeaderIndex(TarHeader *header);
        public:
            TarFS(uint8_t *data) : FileSystem(NULL), fileHandleCounter(0), data(data)
            {
                Initialize(0, 0);
            }

            virtual void Initialize(uint64_t sector, uint64_t sectorCount) override;

            virtual bool Exists(const char *path) override;

            virtual FileSystemHandle GetFileHandle(const char *path) override;
            virtual void DisposeFileHandle(FileSystemHandle handle) override;
            virtual ::FileSystem::FileHandleType FileHandleType(FileSystemHandle handle) override;

            virtual uint64_t FileLength(FileSystemHandle handle) override;

            virtual uint64_t ReadFile(FileSystemHandle handle, void *buffer, uint64_t cursor, uint64_t size) override;
            virtual uint64_t WriteFile(FileSystemHandle handle, const void *buffer, uint64_t cursor, uint64_t size) override;

            virtual void DebugListDirectories() override;

            virtual const char *VolumeName() override;

            virtual ::FileSystem::FileSystemStat Stat(FileSystemHandle handle) override;
        };
    }
}

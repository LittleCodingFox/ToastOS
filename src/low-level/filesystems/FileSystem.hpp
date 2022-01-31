#pragma once
#include <stdint.h>
#include <dirent.h>
#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>
#include "lock.hpp"
#include "devicemanager/GenericIODevice.hpp"
#include "gpt/GPT.hpp"
#include "frg/string.hpp"
#include "frg_allocator.hpp"

#define INVALID_FILE_HANDLE         0xFFFFFFFFFFFFFFFF
#define ROOT_SPECIAL_FILE_HANDLE    (INVALID_FILE_HANDLE - 1)

enum FileHandleType
{
    FILE_HANDLE_UNKNOWN,
    FILE_HANDLE_DIRECTORY,
    FILE_HANDLE_FILE,
    FILE_HANDLE_SYMLINK,
    FILE_HANDLE_CHARDEVICE
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

    virtual int FileHandleType(FileSystemHandle handle) = 0;

    virtual uint64_t FileLength(FileSystemHandle handle) = 0;

    virtual frg::string<frg_allocator> FileLink(FileSystemHandle handle) = 0;

    virtual uint64_t ReadFile(FileSystemHandle handle, void *buffer, uint64_t cursor, uint64_t size) = 0;

    virtual uint64_t WriteFile(FileSystemHandle handle, const void *buffer, uint64_t cursor, uint64_t size) = 0;

    virtual bool Exists(const char *path) = 0;

    virtual void DebugListDirectories() = 0;

    virtual stat Stat(FileSystemHandle handle) = 0;

    virtual dirent *ReadEntries(FileSystemHandle handle) = 0;

    virtual void CloseDir(FileSystemHandle handle) = 0;
};

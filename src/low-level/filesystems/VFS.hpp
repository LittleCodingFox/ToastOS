#pragma once

#include <stdint.h>

#include "FileSystem.hpp"
#include <frg/manual_box.hpp>
#include <frg/vector.hpp>
#include <frg_allocator.hpp>
#include "process/Process.hpp"

namespace FileSystem
{
    typedef uint64_t FILE_HANDLE;

    #define INVALID_FILE_HANDLE 0xFFFFFFFFFFFFFFFF

    class VFS
    {
    private:
        struct MountPoint
        {
            const char *path;
            FileSystem *fileSystem;
        };

        struct FileHandle
        {
            MountPoint *mountPoint;
            frg::string<frg_allocator> path;
            FileSystemHandle fsHandle;
            uint64_t length;
            uint64_t cursor;
            bool isValid;
            ::FileSystem::FileHandleType fileType;
            uint64_t ID;
            FileSystemStat stat;
        };

        frg::vector<MountPoint, frg_allocator> mountPoints;
        frg::vector<FileHandle, frg_allocator> fileHandles;
        uint64_t fileHandleCounter;

        FileHandle *GetFileHandle(FILE_HANDLE handle);
        FileHandle *NewFileHandle();
    public:
        VFS();
        void AddMountPoint(const char *path, FileSystem *fileSystem);
        void RemoveMountPoint(const char *path);

        FILE_HANDLE OpenFile(const char *path, ProcessInfo *currentProcess);
        void CloseFile(FILE_HANDLE handle);
        ::FileSystem::FileHandleType FileType(FILE_HANDLE handle);

        uint64_t FileLength(FILE_HANDLE handle);
        uint64_t FileOffset(FILE_HANDLE handle);
        uint64_t ReadFile(FILE_HANDLE handle, void *buffer, uint64_t length);
        uint64_t WriteFile(FILE_HANDLE handle, const void *buffer, uint64_t length);
        uint64_t SeekFile(FILE_HANDLE handle, uint64_t cursor);
        uint64_t SeekFileBegin(FILE_HANDLE handle);
        uint64_t SeekFileEnd(FILE_HANDLE handle);

        frg::string<frg_allocator> GetFilePath(FILE_HANDLE handle);

        dirent *ReadEntries(FILE_HANDLE handle);
        void CloseDir(FILE_HANDLE handle);

        ::FileSystem::FileSystemStat Stat(FILE_HANDLE handle);
    };

    extern frg::manual_box<VFS> vfs;
}
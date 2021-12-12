#include <sys/stat.h>
#include "VFS.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "string/stringutils.hpp"

namespace FileSystem
{
    frg::manual_box<VFS> vfs;

    VFS::VFS() : fileHandleCounter(0) {}

    VFS::FileHandle *VFS::GetFileHandle(FILE_HANDLE handle)
    {
        if(handle == INVALID_FILE_HANDLE || handle >= fileHandleCounter)
        {
            return NULL;
        }

        for(uint64_t i = 0; i < fileHandles.size(); i++)
        {
            if(fileHandles[i].ID == handle && fileHandles[i].isValid)
            {
                return &fileHandles[i];
            }
        }

        return NULL;
    }

    VFS::FileHandle *VFS::NewFileHandle()
    {
        for(uint64_t i = 0; i < fileHandles.size(); i++)
        {
            if(fileHandles[i].isValid == false)
            {
                return &fileHandles[i];
            }
        }

        FileHandle handle;
        handle.ID = fileHandleCounter++;
        handle.cursor = 0;
        handle.fsHandle = 0;
        handle.mountPoint = NULL;
        handle.length = 0;
        handle.isValid = false;

        return &fileHandles.push_back(handle);
    }

    void VFS::AddMountPoint(const char *path, FileSystem *fileSystem)
    {
        for(uint64_t i = 0; i < mountPoints.size(); i++)
        {
            if(strcmp(mountPoints[i].path, path) == 0)
            {
                DEBUG_OUT("[VFS] Duplicate mount point found for '%s'", path);

                return;
            }
        }

        MountPoint mountPoint;
        mountPoint.path = path;
        mountPoint.fileSystem = fileSystem;

        mountPoints.push_back(mountPoint);

        DEBUG_OUT("[VFS] Added mount point '%s'", path);
    }

    void VFS::RemoveMountPoint(const char *path)
    {
        for(uint64_t i = 0; i < mountPoints.size(); i++)
        {
            if(strcmp(mountPoints[i].path, path) == 0)
            {
                for(uint64_t j = 0; j < fileHandles.size(); j++)
                {
                    if(fileHandles[j].mountPoint == &mountPoints[i])
                    {
                        fileHandles[j].isValid = false;
                        fileHandles[j].mountPoint = NULL;
                    }
                }

                DEBUG_OUT("[VFS] Removed mount point at '%s'", path);

                //TODO: Erase
                //mountPoints.;

                return;
            }
        }
    }

    dirent *VFS::ReadEntries(FILE_HANDLE handle)
    {
        FileHandle *fileHandle = GetFileHandle(handle);

        if(fileHandle == NULL)
        {
            return NULL;
        }

        return fileHandle->mountPoint->fileSystem->ReadEntries(fileHandle->fsHandle);
    }

    frg::string<frg_allocator> VFS::GetFilePath(FILE_HANDLE handle)
    {
        FileHandle *fileHandle = GetFileHandle(handle);

        if(fileHandle == NULL)
        {
            return "";
        }

        return fileHandle->path;
    }

    void VFS::CloseDir(FILE_HANDLE handle)
    {
        FileHandle *fileHandle = GetFileHandle(handle);

        if(fileHandle == NULL)
        {
            return;
        }

        fileHandle->mountPoint->fileSystem->CloseDir(fileHandle->fsHandle);
    }

    FILE_HANDLE VFS::OpenFile(const char *path, ProcessInfo *currentProcess)
    {
        frg::string<frg_allocator> targetPath = path;

        if(targetPath.size() > 0 && targetPath[0] != '/' && currentProcess != NULL)
        {
            targetPath = currentProcess->cwd + targetPath;
        }

        if(targetPath.size() == 0)
        {
            return INVALID_FILE_HANDLE;
        }

        for(auto &mountPoint : mountPoints)
        {
            if(targetPath == mountPoint.path) //Virtual directory
            {
                FileHandle *handle = NewFileHandle();

                handle->path = "";
                handle->fsHandle = INVALID_FILE_HANDLE;
                handle->mountPoint = &mountPoint;
                handle->fileType = FILE_HANDLE_DIRECTORY;
                handle->cursor = 0;
                handle->length = 0;
                handle->isValid = true;

                FileSystemStat stat = {0};

                stat.blksize = 512;
                stat.mode = S_IFDIR | S_IRWXU;
                stat.nlink = 1;

                handle->stat = stat;

                return handle->ID;
            }

            if(strstr(targetPath.data(), mountPoint.path) == targetPath.data())
            {
                uint64_t length = targetPath.size() - strlen(mountPoint.path);

                char *buffer = new char[length + 1];

                memcpy(buffer, targetPath.data() + strlen(mountPoint.path), length);

                buffer[length] = '\0';

                auto mountHandle = mountPoint.fileSystem->GetFileHandle(buffer);

                if(mountHandle != 0)
                {
                    FileHandle *handle = NewFileHandle();

                    handle->path = buffer;
                    handle->fsHandle = mountHandle;
                    handle->mountPoint = &mountPoint;
                    handle->fileType = mountPoint.fileSystem->FileHandleType(handle->fsHandle);
                    handle->cursor = 0;
                    handle->length = mountPoint.fileSystem->FileLength(handle->fsHandle);
                    handle->isValid = true;
                    handle->stat = mountPoint.fileSystem->Stat(handle->fsHandle);

                    delete [] buffer;

                    return handle->ID;
                }

                delete [] buffer;
            }
        }

        if(targetPath.size() > 0 && targetPath[targetPath.size() - 1] != '/')
        {
            targetPath += "/";

            return OpenFile(targetPath.data(), currentProcess);
        }

        return INVALID_FILE_HANDLE;
    }

    void VFS::CloseFile(FILE_HANDLE handle)
    {
        FileHandle *fileHandle = GetFileHandle(handle);

        if(fileHandle != NULL)
        {
            fileHandle->isValid = false;
        }
    }

    ::FileSystem::FileHandleType VFS::FileType(FILE_HANDLE handle)
    {
        FileHandle *fileHandle = GetFileHandle(handle);

        if(fileHandle != NULL && fileHandle->isValid)
        {
            return fileHandle->fileType;
        }

        return FILE_HANDLE_UNKNOWN;
    }

    uint64_t VFS::FileOffset(FILE_HANDLE handle)
    {
        FileHandle *fileHandle = GetFileHandle(handle);

        if(fileHandle == NULL || fileHandle->isValid == false)
        {
            return 0;
        }

        return fileHandle->cursor;
    }

    uint64_t VFS::ReadFile(FILE_HANDLE handle, void *buffer, uint64_t length)
    {
        FileHandle *fileHandle = GetFileHandle(handle);

        if(fileHandle == NULL || fileHandle->isValid == false)
        {
            return 0;
        }

        if(fileHandle->cursor + length > fileHandle->length)
        {
            length = fileHandle->length - fileHandle->cursor;
        }

        uint64_t cursor = fileHandle->cursor;

        fileHandle->cursor += length;
        
        return fileHandle->mountPoint->fileSystem->ReadFile(fileHandle->fsHandle, buffer, cursor, length);
    }

    uint64_t VFS::WriteFile(FILE_HANDLE handle, const void *buffer, uint64_t length)
    {
        FileHandle *fileHandle = GetFileHandle(handle);

        if(fileHandle == NULL || fileHandle->isValid == false)
        {
            return 0;
        }

        uint64_t cursor = fileHandle->cursor;

        fileHandle->cursor += length;

        return fileHandle->mountPoint->fileSystem->WriteFile(fileHandle->fsHandle, buffer, cursor, length);
    }
    
    uint64_t VFS::SeekFile(FILE_HANDLE handle, uint64_t cursor)
    {
        FileHandle *fileHandle = GetFileHandle(handle);

        if(fileHandle == NULL || fileHandle->isValid == false)
        {
            return 0;
        }

        if(cursor > fileHandle->length)
        {
            return 0;
        }

        fileHandle->cursor = cursor;

        return cursor;
    }

    uint64_t VFS::SeekFileBegin(FILE_HANDLE handle)
    {
        FileHandle *fileHandle = GetFileHandle(handle);

        if(fileHandle == NULL || fileHandle->isValid == false)
        {
            return 0;
        }

        fileHandle->cursor = 0;

        return fileHandle->cursor;
    }

    uint64_t VFS::SeekFileEnd(FILE_HANDLE handle)
    {
        FileHandle *fileHandle = GetFileHandle(handle);

        if(fileHandle == NULL || fileHandle->isValid == false)
        {
            return 0;
        }

        fileHandle->cursor = fileHandle->length;

        return fileHandle->cursor;
    }

    uint64_t VFS::FileLength(FILE_HANDLE handle)
    {
        FileHandle *fileHandle = GetFileHandle(handle);

        if(fileHandle == NULL || fileHandle->isValid == false)
        {
            return 0;
        }

        return fileHandle->length;
    }

    ::FileSystem::FileSystemStat VFS::Stat(FILE_HANDLE handle)
    {
        FileHandle *fileHandle = GetFileHandle(handle);

        if(fileHandle == NULL || fileHandle->isValid == false)
        {
            return {0};
        }

        return fileHandle->stat;
    }
}

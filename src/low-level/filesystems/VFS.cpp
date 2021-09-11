#include "VFS.hpp"
#include "debug.hpp"

namespace FileSystem
{
    VFS vfs;

    VFS::VFS() : fileHandleCounter(0) {}

    VFS::FileHandle *VFS::GetFileHandle(FILE_HANDLE handle)
    {
        if(handle == INVALID_FILE_HANDLE || handle >= fileHandleCounter)
        {
            return NULL;
        }

        for(uint64_t i = 0; i < fileHandles.length(); i++)
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
        for(uint64_t i = 0; i < fileHandles.length(); i++)
        {
            if(fileHandles[i].isValid == false)
            {
                return &fileHandles[i];
            }
        }

        FileHandle handle;
        handle.ID = fileHandleCounter++;
        handle.cursor = 0;
        handle.mountPoint = NULL;
        handle.path = NULL;
        handle.length = 0;
        handle.isValid = false;

        fileHandles.add(handle);

        return &fileHandles[fileHandles.length() - 1];
    }

    void VFS::AddMountPoint(const char *path, FileSystem *fileSystem)
    {
        for(uint64_t i = 0; i < mountPoints.length(); i++)
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

        mountPoints.add(mountPoint);

        DEBUG_OUT("[VFS] Added mount point '%s'", path);
    }

    void VFS::RemoveMountPoint(const char *path)
    {
        for(uint64_t i = 0; i < mountPoints.length(); i++)
        {
            if(strcmp(mountPoints[i].path, path) == 0)
            {
                for(uint64_t j = 0; j < fileHandles.length(); j++)
                {
                    if(fileHandles[j].mountPoint == &mountPoints[i])
                    {
                        fileHandles[j].isValid = false;
                        fileHandles[j].mountPoint = NULL;
                    }
                }

                DEBUG_OUT("[VFS] Removed mount point at '%s'", path);

                mountPoints.remove(i);

                return;
            }
        }
    }

    FILE_HANDLE VFS::OpenFile(const char *path)
    {
        uint64_t pathLen = strlen(path);

        for(uint64_t i = 0; i < mountPoints.length(); i++)
        {
            if(strstr(path, mountPoints[i].path))
            {
                uint64_t mountPointLen = strlen(mountPoints[i].path);
                char *innerPath = (char *)malloc(pathLen - mountPointLen + 1);

                memcpy(innerPath, &path[strlen(mountPoints[i].path)], pathLen - mountPointLen);

                innerPath[pathLen - mountPointLen] = '\0';

                if(mountPoints[i].fileSystem->Exists(innerPath))
                {
                    FileHandle *handle = NewFileHandle();

                    handle->path = innerPath;
                    handle->mountPoint = &mountPoints[i];
                    handle->fileType = FILE_TYPE_FILE;
                    handle->cursor = 0;
                    handle->length = mountPoints[i].fileSystem->FileLength(innerPath);
                    handle->isValid = true;

                    return handle->ID;
                }

                return INVALID_FILE_HANDLE;                
            }
        }
    }

    void VFS::CloseFile(FILE_HANDLE handle)
    {
        FileHandle *fileHandle = GetFileHandle(handle);

        if(fileHandle != NULL)
        {
            fileHandle->isValid = false;
        }
    }

    ::FileSystem::FileType VFS::FileType(FILE_HANDLE handle)
    {
        FileHandle *fileHandle = GetFileHandle(handle);

        if(fileHandle != NULL && fileHandle->isValid)
        {
            return fileHandle->fileType;
        }

        return FILE_TYPE_UNKNOWN;
    }

    uint64_t VFS::ReadFile(FILE_HANDLE handle, void *buffer, uint64_t length)
    {
        FileHandle *fileHandle = GetFileHandle(handle);

        if(fileHandle == NULL || fileHandle->isValid == false)
        {
            return 0;
        }

        if(fileHandle->cursor + length >= fileHandle->length)
        {
            length = fileHandle->length - fileHandle->cursor;
        }

        uint64_t cursor = fileHandle->cursor;

        fileHandle->cursor += length;

        return fileHandle->mountPoint->fileSystem->ReadFile(fileHandle->path, buffer, cursor, length);
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

        return fileHandle->mountPoint->fileSystem->WriteFile(fileHandle->path, buffer, cursor, length);
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
}

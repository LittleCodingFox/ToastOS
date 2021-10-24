#include <sys/stat.h>
#include "VFS.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "string/stringutils.hpp"

namespace FileSystem
{
    frg::manual_box<VFS> vfs;

    VFS::VFS() : fileHandleCounter(0) {}

    frg::string<frg_allocator> VFS::ResolvePath(const frg::string<frg_allocator> &path)
    {
        if(path == "." || path == "..")
        {
            return path;
        }

        frg::vector<frg::string<frg_allocator>, frg_allocator> parts = SplitString(path, '/');

        //TODO: Erase impl in frg vector
        frg::vector<frg::string<frg_allocator>, frg_allocator> validParts;

        for(int64_t i = parts.size() - 1; i >= 0; i--)
        {
            if(parts[i] == ".." && i > 0)
            {
                i--;

                continue;
            }
            else if(parts[i] == ".")
            {
                continue;
            }

            validParts.push_back(parts[i]);
        }

        parts.clear();

        for(uint64_t i = 0; i < validParts.size(); i++)
        {
            if(i > 0)
            {
                parts.push_back("/");
            }

            parts.push_back(validParts[i]);
        }

        frg::string<frg_allocator> outValue;

        for(int64_t i = parts.size() - 1; i >= 0; i--)
        {
            outValue += parts[i];
        }

        return outValue;
    }

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
        handle.path = NULL;
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

    void VFS::CloseDir(FILE_HANDLE handle)
    {
        FileHandle *fileHandle = GetFileHandle(handle);

        if(fileHandle == NULL)
        {
            return;
        }

        fileHandle->mountPoint->fileSystem->CloseDir(fileHandle->fsHandle);
    }

    FILE_HANDLE VFS::OpenFile(const char *path)
    {
        frg::string<frg_allocator> targetPath = ResolvePath(path);

        if(targetPath.size() == 0)
        {
            return INVALID_FILE_HANDLE;
        }

        if(targetPath[0] != '/')
        {
            ProcessInfo *process = globalProcessManager->CurrentProcess();

            if(targetPath == ".")
            {
                return OpenFile(process->cwd.data());
            }

            if(targetPath == "..")
            {
                frg::string<frg_allocator> cwd = process->cwd;

                bool needsSlash = cwd[cwd.size() - 1] == '/';

                if(needsSlash)
                {
                    char *buffer = new char[cwd.size()];

                    memcpy(buffer, cwd.data(), cwd.size() - 1);

                    buffer[cwd.size() - 1] = '\0';

                    cwd = buffer;

                    delete [] buffer;
                }

                char *ptr = strrchr(cwd.data(), '/');

                if(ptr == NULL || ptr == cwd.data())
                {
                    return INVALID_FILE_HANDLE;
                }

                uint64_t length = ((uint64_t)ptr - (uint64_t)&cwd[0]) + (needsSlash ? 1 : 0);

                char *buffer = new char[length + 1];

                memcpy(buffer, cwd.data(), length - (needsSlash ? 1 : 0));

                if(needsSlash)
                {
                    buffer[length - 1] = '/';
                }

                buffer[length] = '\0';

                FILE_HANDLE handle = OpenFile(buffer);

                delete [] buffer;

                return handle;
            }

            targetPath = process->cwd + (process->cwd[process->cwd.size() - 1] != '/' ? "/" : "") + targetPath;
        }

        DEBUG_OUT("open path for %s", &targetPath[0]);

        uint64_t pathLen = strlen(&targetPath[0]);

        for(uint64_t i = 0; i < mountPoints.size(); i++)
        {
            MountPoint *mountPoint = &mountPoints[i];

            if(targetPath == mountPoint->path) //Virtual directory
            {
                FileHandle *handle = NewFileHandle();

                handle->path = "";
                handle->fsHandle = INVALID_FILE_HANDLE;
                handle->mountPoint = mountPoint;
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

            if(strstr(&targetPath[0], mountPoint->path))
            {
                uint64_t mountPointLen = strlen(mountPoint->path);
                char *innerPath = (char *)malloc(pathLen - mountPointLen + 1);

                memcpy(innerPath, &targetPath[strlen(mountPoint->path)], pathLen - mountPointLen);

                innerPath[pathLen - mountPointLen] = '\0';

                if(mountPoint->fileSystem->Exists(innerPath))
                {
                    FileHandle *handle = NewFileHandle();

                    handle->path = innerPath;
                    handle->fsHandle = mountPoint->fileSystem->GetFileHandle(innerPath);
                    handle->mountPoint = mountPoint;
                    handle->fileType = mountPoint->fileSystem->FileHandleType(handle->fsHandle);
                    handle->cursor = 0;
                    handle->length = mountPoints[i].fileSystem->FileLength(handle->fsHandle);
                    handle->isValid = true;
                    handle->stat = mountPoint->fileSystem->Stat(handle->fsHandle);

                    return handle->ID;
                }
                else
                {
                    free(innerPath);
                }
            }
        }

        if(targetPath.size() > 0 && targetPath[targetPath.size() - 1] != '/')
        {
            targetPath += "/";

            return OpenFile(targetPath.data());
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

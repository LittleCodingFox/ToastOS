#include "VFS.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "string/stringutils.hpp"

box<VFS> vfs;

uint64_t VirtualFile::Length() const
{
    return length != NULL ? length() : 0;
}

uint64_t VirtualFile::Read(void *buffer, uint64_t cursor, uint64_t size, int *error)
{
    return read != NULL ? read(buffer, cursor, size, error) : 0;
}

uint64_t VirtualFile::Write(const void *buffer, uint64_t cursor, uint64_t size, int *error)
{
    return write != NULL ? write(buffer, cursor, size, error) : 0;
}

VFS::VFS() : fileHandleCounter(0) {}

void VFS::AddVirtualFile(const VirtualFile &file)
{
    auto newFile = new VirtualFile(file);

    virtualFiles.push_back(newFile);
}

void VFS::RemoveVirtualFile(const string &path)
{
    //TODO
}

VFS::FileHandle *VFS::GetFileHandle(FILE_HANDLE handle)
{
    if(handle == INVALID_FILE_HANDLE || handle >= fileHandleCounter)
    {
        return NULL;
    }

    for(uint64_t i = 0; i < fileHandles.size(); i++)
    {
        if(fileHandles[i]->ID == handle && fileHandles[i]->isValid)
        {
            return fileHandles[i];
        }
    }

    return NULL;
}

VFS::FileHandle *VFS::NewFileHandle()
{
    for(uint64_t i = 0; i < fileHandles.size(); i++)
    {
        if(fileHandles[i]->isValid == false)
        {
            auto handle = fileHandles[i];
            
            handle->cursor = 0;
            handle->fileType = FILE_HANDLE_UNKNOWN;
            handle->flags = 0;
            handle->fsHandle = INVALID_FILE_HANDLE;
            handle->length = 0;
            handle->mountPoint = NULL;
            handle->virtualFile = NULL;

            return handle;
        }
    }

    auto handle = new FileHandle();

    handle->ID = fileHandleCounter++;

    return fileHandles.push_back(handle);
}

void VFS::AddMountPoint(const char *path, FileSystem *fileSystem)
{
    for(uint64_t i = 0; i < mountPoints.size(); i++)
    {
        if(strcmp(mountPoints[i]->path, path) == 0)
        {
            DEBUG_OUT("[VFS] Duplicate mount point found for '%s'", path);

            return;
        }
    }

    MountPoint *mountPoint = new MountPoint();
    mountPoint->path = path;
    mountPoint->fileSystem = fileSystem;

    mountPoints.push_back(mountPoint);

    DEBUG_OUT("[VFS] Added mount point '%s'", path);
}

void VFS::RemoveMountPoint(const char *path)
{
    for(uint64_t i = 0; i < mountPoints.size(); i++)
    {
        if(strcmp(mountPoints[i]->path, path) == 0)
        {
            for(uint64_t j = 0; j < fileHandles.size(); j++)
            {
                if(fileHandles[j]->isValid && fileHandles[j]->mountPoint == mountPoints[i])
                {
                    fileHandles[j]->isValid = false;
                    fileHandles[j]->mountPoint = NULL;
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

    if(fileHandle == NULL || fileHandle->isValid == false)
    {
        return NULL;
    }

    if(fileHandle->virtualFile != NULL)
    {
        return NULL;
    }

    return fileHandle->mountPoint->fileSystem->ReadEntries(fileHandle->fsHandle);
}

string VFS::GetFilePath(FILE_HANDLE handle)
{
    FileHandle *fileHandle = GetFileHandle(handle);

    if(fileHandle == NULL || fileHandle->isValid == false)
    {
        return "";
    }

    return fileHandle->path;
}

void VFS::CloseDir(FILE_HANDLE handle)
{
    FileHandle *fileHandle = GetFileHandle(handle);

    if(fileHandle == NULL || fileHandle->isValid == false)
    {
        return;
    }

    if(fileHandle->virtualFile != NULL)
    {
        return;
    }

    fileHandle->mountPoint->fileSystem->CloseDir(fileHandle->fsHandle);
}

uint32_t VFS::FileFlags(FILE_HANDLE handle)
{
    auto fileHandle = GetFileHandle(handle);

    if(fileHandle == NULL || fileHandle->isValid == false)
    {
        return 0;
    }

    return fileHandle->flags;
}

void VFS::SetFileFlags(FILE_HANDLE handle, uint32_t flags)
{
    auto fileHandle = GetFileHandle(handle);

    if(fileHandle == NULL || fileHandle->isValid == false)
    {
        return;
    }

    fileHandle->flags = flags;
}

FILE_HANDLE VFS::OpenFile(const char *path, uint32_t flags, Process *currentProcess, int *error)
{
    string targetPath = path;

    if(targetPath.size() > 0 && currentProcess != NULL)
    {
        if(targetPath.size() >= 2 && targetPath[0] == '.' && targetPath[1] == '/')
        {
            auto pathView = frg::string_view(targetPath);

            targetPath = currentProcess->cwd + pathView.sub_string(2, pathView.size() - 2);
        }
        else if(targetPath[0] != '/')
        {
            targetPath = currentProcess->cwd + targetPath;
        }
    }

    //DEBUG_OUT("Opening file %s (flags: 0x%x)", targetPath.data(), flags);

    if(targetPath.size() == 0)
    {
        return INVALID_FILE_HANDLE;
    }

    for(auto &file : virtualFiles)
    {
        if(file->path == targetPath)
        {
            if((flags & O_DIRECTORY) == O_DIRECTORY && file->type != FILE_HANDLE_DIRECTORY)
            {
                *error = ENOTDIR;

                return INVALID_FILE_HANDLE;
            }

            FileHandle *handle = NewFileHandle();

            handle->path = file->path;
            handle->fsHandle = INVALID_FILE_HANDLE;
            handle->mountPoint = NULL;
            handle->cursor = 0;
            handle->length = 0;
            handle->virtualFile = file;
            handle->fileType = file->type;
            handle->isValid = true;
            handle->flags = flags;

            struct stat stat = {0};

            stat.st_blksize = 512;

            switch(file->type)
            {
                case FILE_HANDLE_FILE:

                    stat.st_mode = S_IFREG | S_IRWXU;

                    break;

                case FILE_HANDLE_DIRECTORY:

                    stat.st_mode = S_IFDIR | S_IRWXU;

                    break;

                case FILE_HANDLE_CHARDEVICE:

                    stat.st_mode = S_IFCHR | S_IRWXU;

                    break;

                case FILE_HANDLE_SYMLINK:

                    stat.st_mode = S_IFLNK | S_IRWXU;

                    break;

                case FILE_HANDLE_UNKNOWN:

                    stat.st_mode = 0;

                    break;
            }

            stat.st_nlink = 1;
            stat.st_ino = 0;
            stat.st_dev = 0;

            handle->stat = stat;

            return handle->ID;
        }
    }

    for(auto &mountPoint : mountPoints)
    {
        if(targetPath == mountPoint->path) //Virtual directory
        {
            FileHandle *handle = NewFileHandle();

            handle->path = "";
            handle->fsHandle = mountPoint->fileSystem->GetFileHandle("", flags);
            handle->mountPoint = mountPoint;
            handle->fileType = FILE_HANDLE_DIRECTORY;
            handle->virtualFile = NULL;
            handle->cursor = 0;
            handle->length = 0;
            handle->isValid = true;
            handle->flags = flags;
            handle->stat = mountPoint->fileSystem->Stat(handle->fsHandle);

            return handle->ID;
        }

        if(strstr(targetPath.data(), mountPoint->path) == targetPath.data())
        {
            uint64_t length = targetPath.size() - strlen(mountPoint->path);

            char *buffer = new char[length + 1];

            memcpy(buffer, targetPath.data() + strlen(mountPoint->path), length);

            buffer[length] = '\0';

            auto mountHandle = mountPoint->fileSystem->GetFileHandle(buffer, flags);

            if(mountHandle != 0)
            {
                if(mountPoint->fileSystem->FileHandleType(mountHandle) == FILE_HANDLE_SYMLINK)
                {
                    auto path = mountPoint->fileSystem->FileLink(mountHandle);

                    if(path.size() > 0 && path[0] == '/')
                    {
                        return OpenFile(path.data(), flags, currentProcess, error);
                    }
                    else
                    {
                        char *newPath = NULL;
                        char *ptr = strrchr(buffer, '/');
                        string linkPath;

                        if(ptr != NULL)
                        {
                            int length = (int)(ptr - buffer) + 1;

                            newPath = new char[length + strlen(buffer) + 1];

                            memcpy(newPath, buffer, length);
                            memcpy(newPath + length, path.data(), strlen(path.data()));

                            newPath[length + strlen(path.data())] = '\0';

                            linkPath = newPath;

                            delete [] newPath;
                        }

                        auto nextPath = string(mountPoint->path) + linkPath;

                        return OpenFile(nextPath.data(), flags, currentProcess, error);
                    }
                }

                if((flags & O_DIRECTORY) == O_DIRECTORY && mountPoint->fileSystem->FileHandleType(mountHandle) != FILE_HANDLE_DIRECTORY)
                {
                    *error = ENOTDIR;

                    return INVALID_FILE_HANDLE;
                }

                FileHandle *handle = NewFileHandle();

                handle->path = buffer;
                handle->fsHandle = mountHandle;
                handle->mountPoint = mountPoint;
                handle->fileType = mountPoint->fileSystem->FileHandleType(handle->fsHandle);
                handle->virtualFile = NULL;
                handle->cursor = 0;
                handle->length = mountPoint->fileSystem->FileLength(handle->fsHandle);
                handle->isValid = true;
                handle->stat = mountPoint->fileSystem->Stat(handle->fsHandle);
                handle->flags = flags;

                delete [] buffer;

                return handle->ID;
            }

            delete [] buffer;
        }
    }

    if(targetPath.size() > 0 && targetPath[targetPath.size() - 1] != '/')
    {
        targetPath += "/";

        return OpenFile(targetPath.data(), flags, currentProcess, error);
    }

    return INVALID_FILE_HANDLE;
}

bool VFS::ReadLink(const char *path, string &link, Process *currentProcess)
{
    string targetPath = path;

    if(targetPath.size() > 0 && currentProcess != NULL)
    {
        if(targetPath.size() >= 2 && targetPath[0] == '.' && targetPath[1] == '/')
        {
            auto pathView = frg::string_view(targetPath);

            targetPath = currentProcess->cwd + pathView.sub_string(2, pathView.size() - 2);
        }
        else if(targetPath[0] != '/')
        {
            targetPath = currentProcess->cwd + targetPath;
        }
    }

    if(targetPath.size() == 0)
    {
        return false;
    }

    for(auto &mountPoint : mountPoints)
    {
        if(targetPath == mountPoint->path) //Virtual directory
        {
            return false;
        }

        if(strstr(targetPath.data(), mountPoint->path) == targetPath.data())
        {
            uint64_t length = targetPath.size() - strlen(mountPoint->path);

            char *buffer = new char[length + 1];

            memcpy(buffer, targetPath.data() + strlen(mountPoint->path), length);

            buffer[length] = '\0';

            auto mountHandle = mountPoint->fileSystem->GetFileHandle(buffer, O_RDONLY);

            if(mountHandle != 0)
            {
                if(mountPoint->fileSystem->FileHandleType(mountHandle) == FILE_HANDLE_SYMLINK)
                {
                    link = mountPoint->fileSystem->FileLink(mountHandle);

                    mountPoint->fileSystem->DisposeFileHandle(mountHandle);

                    delete [] buffer;

                    return true;
                }

                mountPoint->fileSystem->DisposeFileHandle(mountHandle);
            }

            delete [] buffer;
        }
    }

    if(targetPath.size() > 0 && targetPath[targetPath.size() - 1] != '/')
    {
        targetPath += "/";

        return ReadLink(targetPath.data(), link, currentProcess);
    }

    return false;
}

void VFS::CloseFile(FILE_HANDLE handle)
{
    FileHandle *fileHandle = GetFileHandle(handle);

    if(fileHandle != NULL)
    {
        fileHandle->isValid = false;
    }
}

int VFS::FileType(FILE_HANDLE handle)
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

uint64_t VFS::ReadFile(FILE_HANDLE handle, void *buffer, uint64_t length, int *error)
{
    FileHandle *fileHandle = GetFileHandle(handle);

    if(fileHandle == NULL || fileHandle->isValid == false)
    {
        *error = ENOENT;

        return 0;
    }

    if(fileHandle->fileType == FILE_HANDLE_DIRECTORY)
    {
        *error = EISDIR;

        return 0;
    }

    if(fileHandle->cursor + length > fileHandle->length)
    {
        length = fileHandle->length - fileHandle->cursor;
    }

    uint64_t cursor = fileHandle->cursor;
    uint64_t done = 0;

    if(fileHandle->virtualFile != NULL)
    {
        done = fileHandle->virtualFile->Read(buffer, cursor, length, error);
    }
    else
    {
        done = fileHandle->mountPoint->fileSystem->ReadFile(fileHandle->fsHandle, buffer, cursor, length);
    }

    fileHandle->cursor += done;

    return done;
}

uint64_t VFS::WriteFile(FILE_HANDLE handle, const void *buffer, uint64_t length, int *error)
{
    FileHandle *fileHandle = GetFileHandle(handle);

    if(fileHandle == NULL || fileHandle->isValid == false)
    {
        *error = ENOENT;

        return 0;
    }

    if(fileHandle->fileType == FILE_HANDLE_DIRECTORY)
    {
        *error = EISDIR;

        return 0;
    }

    uint64_t cursor = fileHandle->cursor;
    uint64_t done = 0;

    if(fileHandle->virtualFile != NULL)
    {
        done = fileHandle->virtualFile->Write(buffer, cursor, length, error);
    }
    else
    {
        done = fileHandle->mountPoint->fileSystem->WriteFile(fileHandle->fsHandle, buffer, cursor, length);
    }

    fileHandle->cursor += done;
    fileHandle->length += done;

    return done;
}

uint64_t VFS::SeekFile(FILE_HANDLE handle, uint64_t cursor, int *error)
{
    FileHandle *fileHandle = GetFileHandle(handle);

    if(fileHandle == NULL || fileHandle->isValid == false)
    {
        *error = ENOENT;

        return 0;
    }

    if(fileHandle->fileType == FILE_HANDLE_DIRECTORY)
    {
        *error = EISDIR;

        return 0;
    }

    if(cursor > fileHandle->length)
    {
        return 0;
    }

    fileHandle->cursor = cursor;

    return cursor;
}

uint64_t VFS::SeekFileBegin(FILE_HANDLE handle, int *error)
{
    FileHandle *fileHandle = GetFileHandle(handle);

    if(fileHandle == NULL || fileHandle->isValid == false)
    {
        *error = ENOENT;

        return 0;
    }

    if(fileHandle->fileType == FILE_HANDLE_DIRECTORY)
    {
        *error = EISDIR;

        return 0;
    }

    fileHandle->cursor = 0;

    return fileHandle->cursor;
}

uint64_t VFS::SeekFileEnd(FILE_HANDLE handle, int *error)
{
    FileHandle *fileHandle = GetFileHandle(handle);

    if(fileHandle == NULL || fileHandle->isValid == false)
    {
        *error = ENOENT;

        return 0;
    }

    if(fileHandle->fileType == FILE_HANDLE_DIRECTORY)
    {
        *error = EISDIR;

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

VFS::FileHandle *VFS::ResolveSymlink(FileHandle *original, uint32_t flags)
{
    if(original->fileType != FILE_HANDLE_SYMLINK || original->virtualFile != NULL)
    {
        return original;
    }

    auto currentProcess = processManager->CurrentProcess();

    int error = 0;

    while(original->fileType == FILE_HANDLE_SYMLINK)
    {
        auto path = original->mountPoint->fileSystem->FileLink(original->fsHandle);

        auto handle = OpenFile(path.data(), flags, currentProcess != NULL ? currentProcess->info : NULL, &error);

        if(handle == INVALID_FILE_HANDLE || error != 0)
        {
            return NULL;
        }

        original = GetFileHandle(handle);
    }

    return original;
}

struct stat VFS::Stat(FILE_HANDLE handle, int *error)
{
    FileHandle *fileHandle = GetFileHandle(handle);

    if(fileHandle == NULL || fileHandle->isValid == false)
    {
        *error = ENOENT;

        return {0};
    }

    return fileHandle->stat;
}

bool VFS::MakeDir(const char *path, mode_t mode, Process *currentProcess)
{
    string targetPath = path;

    if(targetPath.size() > 0 && currentProcess != NULL)
    {
        if(targetPath.size() >= 2 && targetPath[0] == '.' && targetPath[1] == '/')
        {
            auto pathView = frg::string_view(targetPath);

            targetPath = currentProcess->cwd + pathView.sub_string(2, pathView.size() - 2);
        }
        else if(targetPath[0] != '/')
        {
            targetPath = currentProcess->cwd + targetPath;
        }
    }

    if(targetPath.size() == 0)
    {
        return false;
    }

    for(auto &mountPoint : mountPoints)
    {
        if(targetPath == mountPoint->path) //Virtual directory
        {
            return false;
        }

        if(strstr(targetPath.data(), mountPoint->path) == targetPath.data())
        {
            return mountPoint->fileSystem->MakeDir(targetPath.data() + strlen(mountPoint->path), mode);
        }
    }

    return false;
}

bool VFS::Rename(const char *path, const char *newPath, Process *currentProcess)
{
    string targetPath = path;

    if(targetPath.size() > 0 && currentProcess != NULL)
    {
        if(targetPath.size() >= 2 && targetPath[0] == '.' && targetPath[1] == '/')
        {
            auto pathView = frg::string_view(targetPath);

            targetPath = currentProcess->cwd + pathView.sub_string(2, pathView.size() - 2);
        }
        else if(targetPath[0] != '/')
        {
            targetPath = currentProcess->cwd + targetPath;
        }
    }

    if(targetPath.size() == 0)
    {
        return false;
    }

    for(auto &mountPoint : mountPoints)
    {
        if(targetPath == mountPoint->path) //Virtual directory
        {
            return false;
        }

        if(strstr(targetPath.data(), mountPoint->path) == targetPath.data())
        {
            return mountPoint->fileSystem->Rename(targetPath.data() + strlen(mountPoint->path), newPath);
        }
    }

    return false;
}

bool VFS::RemoveDir(const char *path, Process *currentProcess)
{
    string targetPath = path;

    if(targetPath.size() > 0 && currentProcess != NULL)
    {
        if(targetPath.size() >= 2 && targetPath[0] == '.' && targetPath[1] == '/')
        {
            auto pathView = frg::string_view(targetPath);

            targetPath = currentProcess->cwd + pathView.sub_string(2, pathView.size() - 2);
        }
        else if(targetPath[0] != '/')
        {
            targetPath = currentProcess->cwd + targetPath;
        }
    }

    if(targetPath.size() == 0)
    {
        return false;
    }

    for(auto &mountPoint : mountPoints)
    {
        if(targetPath == mountPoint->path) //Virtual directory
        {
            return false;
        }

        if(strstr(targetPath.data(), mountPoint->path) == targetPath.data())
        {
            return mountPoint->fileSystem->RemoveDir(targetPath.data() + strlen(mountPoint->path));
        }
    }

    return false;
}

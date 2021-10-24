#include <sys/stat.h>
#include "tarfs.hpp"
#include "frg/string.hpp"

namespace FileSystem
{
    namespace tarfs
    {
        uint64_t OctToInt(const char *str)
        {
            uint64_t size = 0;

            for(uint64_t j = 11, count = 1; j > 0; j--, count *= 8)
            {
                size += ((str[j - 1] - '0') * count);
            }

            return size;
        }

        uint64_t TarFS::GetHeaderIndex(TarHeader *target)
        {
            uint64_t counter = 0;

            for(auto &header : headers)
            {
                counter++;

                if(header == target)
                {
                    return counter;
                }
            }

            return counter;
        }

        void TarFS::Initialize(uint64_t sector, uint64_t sectorCount)
        {
            uint8_t *ptr = data;

            for(;;)
            {
                TarHeader *header = (TarHeader *)ptr;

                if(header->name[0] == '\0')
                {
                    break;
                }

                uint64_t size = OctToInt(header->size);

                ptr += ((size / 512) + 1) * 512;

                if(size % 512)
                {
                    ptr += 512;
                }

                headers.push_back(header);
            }

            DEBUG_OUT("TARFS: Found %llu headers", headers.size());
        }

        TarFS::FileHandleData *TarFS::GetHandle(FileSystemHandle handle)
        {
            for(auto &file : fileHandles)
            {
                if(file.ID == handle)
                {
                    return &file;
                }
            }

            return NULL;
        }

        bool TarFS::Exists(const char *path)
        {
            for(auto &header : headers)
            {
                if(strcmp(header->name, path) == 0)
                {
                    return true;
                }
            }

            return false;
        }

        ::FileSystem::FileSystemStat TarFS::Stat(FileSystemHandle handle)
        {
            ::FileSystem::FileSystemStat stat = {0};

            auto file = GetHandle(handle);

            if(file == NULL)
            {
                return stat;
            }

            stat.uid = OctToInt(file->header->uid);
            stat.gid = OctToInt(file->header->gid);
            stat.nlink = 1;
            stat.size = file->length;
            stat.blksize = 512;
            stat.blocks = stat.size / stat.blksize + 1;
            stat.ino = GetHeaderIndex(file->header);

            switch(file->header->type)
            {
                case TAR_SYMLINK:

                    stat.mode = S_IFLNK | S_IRWXU;

                    break;

                case TAR_FILE:

                    stat.mode = S_IFMT | S_IRWXU;

                    break;

                case TAR_DIRECTORY:

                    stat.mode = S_IFDIR | S_IRWXU;

                    break;
            }

            return stat;
        }

        FileSystemHandle TarFS::GetFileHandle(const char *path)
        {
            for(auto &file : fileHandles)
            {
                if(strcmp(file.header->name, path) == 0)
                {
                    return file.ID;
                }
            }

            for(auto &header : headers)
            {
                if(strcmp(header->name, path) == 0)
                {
                    if(header->type == TAR_SYMLINK)
                    {
                        DEBUG_OUT("tarfs: file %s is a symlink to %s", header->name, header->link);

                        char *targetPath = header->link;
                        char *newPath = NULL;

                        if(targetPath[0] != '/')
                        {
                            char *ptr = strrchr(header->name, '/');

                            if(ptr != NULL)
                            {
                                int length = (int)(ptr - header->name) + 1;

                                newPath = new char[length + strlen(header->link) + 1];

                                memcpy(newPath, header->name, length);
                                memcpy(newPath + length, header->link, strlen(header->link));

                                newPath[length + strlen(header->link)] = '\0';

                                targetPath = newPath;

                                DEBUG_OUT("tarfs: resolved to %s", targetPath);
                            }
                        }

                        auto handle = GetFileHandle(targetPath);

                        if(newPath != NULL)
                        {
                            delete [] newPath;
                        }

                        return handle;
                    }
                    else
                    {
                        FileHandleData data;

                        data.ID = ++fileHandleCounter;
                        data.header = header;
                        data.length = OctToInt(header->size);

                        if(header->type == TAR_DIRECTORY)
                        {
                            data.currentEntry = 0;

                            dirent current = {0};

                            current.d_ino = GetHeaderIndex(header);
                            current.d_reclen = sizeof(dirent);
                            strcpy(current.d_name, ".");
                            current.d_type = DT_DIR;

                            data.entries.push_back(current);

                            if(strlen(header->name) > 1)
                            {
                                char *temp = new char[strlen(header->name)];

                                memcpy(temp, header->name, strlen(header->name) - 1);

                                temp[strlen(header->name)] = '\0';

                                char *ptr = strrchr(temp, '/');

                                if(ptr != NULL)
                                {
                                    temp[(uint64_t)ptr - (uint64_t)temp] = '\0';

                                    for(auto &previousHeader : headers)
                                    {
                                        if(strcmp(previousHeader->name, temp) == 0)
                                        {
                                            DEBUG_OUT("Found previous header %s for directory %s", previousHeader->name, header->name);

                                            dirent previous = {0};

                                            previous.d_ino = GetHeaderIndex(previousHeader);
                                            previous.d_reclen = sizeof(dirent);

                                            strcpy(previous.d_name, "..");
                                            previous.d_type = DT_DIR;

                                            data.entries.push_back(previous);

                                            break;
                                        }
                                    }
                                }
                                else
                                {
                                    dirent previous = current;

                                    strcpy(previous.d_name, "..");

                                    data.entries.push_back(previous);
                                }

                                delete [] temp;
                            }

                            for(auto &file : headers)
                            {
                                if(strlen(file->name) > strlen(header->name) && strncmp(file->name, header->name, strlen(header->name)) == 0)
                                {
                                    uint64_t length = strlen(file->name) - strlen(header->name);

                                    char *buffer = new char[length + 1];

                                    memcpy(buffer, file->name + strlen(header->name), length);

                                    buffer[length] = '\0';

                                    char *ptr = strchr(buffer, '/');

                                    if(ptr != NULL && ptr != &buffer[length - 1])
                                    {
                                        delete [] buffer;

                                        continue;
                                    }

                                    dirent entry = {0};

                                    entry.d_ino = GetHeaderIndex(file);
                                    entry.d_reclen = sizeof(dirent);

                                    length = strlen(file->name) - strlen(header->name) - (ptr != NULL ? 1 : 0);

                                    memcpy(entry.d_name, &file->name[strlen(header->name)], length);

                                    entry.d_name[length] = '\0';

                                    switch(header->type)
                                    {
                                        case TAR_FILE:

                                            entry.d_type = DT_REG;

                                            break;

                                        case TAR_SYMLINK:

                                            entry.d_type = DT_LNK;

                                            break;

                                        case TAR_DIRECTORY:

                                            entry.d_type = DT_DIR;

                                            break;
                                    }

                                    data.entries.push_back(entry);
                                }
                            }
                        }

                        fileHandles.push_back(data);

                        return data.ID;
                    }
                }
            }

            return 0;
        }

        void TarFS::DisposeFileHandle(FileSystemHandle handle)
        {
            //TODO
        }

        ::FileSystem::FileHandleType TarFS::FileHandleType(FileSystemHandle handle)
        {
            auto file = GetHandle(handle);

            if(file == NULL)
            {
                return FILE_HANDLE_UNKNOWN;
            }

            switch(file->header->type)
            {
                case TAR_DIRECTORY:

                    return FILE_HANDLE_DIRECTORY;

                case TAR_FILE:

                    return FILE_HANDLE_FILE;
            }

            return FILE_HANDLE_UNKNOWN;
        }

        dirent *TarFS::ReadEntries(FileSystemHandle handle)
        {
            auto file = GetHandle(handle);

            if(file == NULL)
            {
                return NULL;
            }

            if(file->currentEntry < file->entries.size())
            {
                dirent *entry = &file->entries[file->currentEntry];

                file->currentEntry++;

                return entry;
            }

            return NULL;
        }

        void TarFS::CloseDir(FileSystemHandle handle)
        {
            auto file = GetHandle(handle);

            if(file == NULL)
            {
                return;
            }

            file->currentEntry = 0;
        }

        uint64_t TarFS::FileLength(FileSystemHandle handle)
        {
            auto file = GetHandle(handle);

            if(file == NULL)
            {
                return 0;
            }

            return file->length;
        }

        uint64_t TarFS::ReadFile(FileSystemHandle handle, void *buffer, uint64_t cursor, uint64_t size)
        {
            auto file = GetHandle(handle);

            if(file == NULL)
            {
                return 0;
            }

            if(cursor > file->length)
            {
                return 0;
            }

            if(size > file->length)
            {
                size = file->length;
            }

            if(cursor + size > file->length)
            {
                size = file->length - cursor;
            }

            memcpy(buffer, (uint8_t *)file->header + 512 + cursor, size);

            return size;
        }

        uint64_t TarFS::WriteFile(FileSystemHandle handle, const void *buffer, uint64_t cursor, uint64_t size)
        {
            return 0;
        }

        void TarFS::DebugListDirectories()
        {
            printf("Listing directories\n");
            
            for(auto &header : headers)
            {
                printf("%s (%llu)\n", header->name, OctToInt(header->size));
            }

            printf("Listed directories\n");
        }

        const char *TarFS::VolumeName()
        {
            return "tarfs";
        }
    }
}

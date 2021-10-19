#include "tarfs.hpp"

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
                    FileHandleData data;

                    data.ID = ++fileHandleCounter;
                    data.header = header;
                    data.length = OctToInt(header->size);

                    fileHandles.push_back(data);

                    return data.ID;
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
            for(auto &file : fileHandles)
            {
                if(file.ID == handle)
                {
                    if(file.header->type == TAR_DIRECTORY)
                    {
                        return FILE_HANDLE_DIRECTORY;
                    }
                    else if(file.header->type == TAR_FILE)
                    {
                        return FILE_HANDLE_FILE;
                    }
                    else
                    {
                        return FILE_HANDLE_UNKNOWN;
                    }
                }
            }

            return FILE_HANDLE_UNKNOWN;
        }

        uint64_t TarFS::FileLength(FileSystemHandle handle)
        {
            for(auto &file : fileHandles)
            {
                if(file.ID == handle)
                {
                    return file.length;
                }
            }

            return 0;
        }

        uint64_t TarFS::ReadFile(FileSystemHandle handle, void *buffer, uint64_t cursor, uint64_t size)
        {
            for(auto &file : fileHandles)
            {
                if(file.ID == handle)
                {
                    if(cursor > file.length)
                    {
                        return 0;
                    }

                    if(size > file.length)
                    {
                        size = file.length;
                    }

                    if(cursor + size > file.length)
                    {
                        size = file.length - cursor;
                    }

                    memcpy(buffer, (uint8_t *)file.header + 512 + cursor, size);

                    return size;
                }
            }

            return 0;
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

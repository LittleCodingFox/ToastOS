#include "tarfs.hpp"
#include "frg/string.hpp"
#include "string/stringutils.hpp"

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

    for(auto &header : headers)
    {
        if(!strstr(header->name, "/"))
        {
            AddInode(header, NULL);
        }
    }
}

void TarFS::AddInode(TarHeader *header, TarFS::Inode *parent)
{
    auto inode = new Inode();

    auto end = strrchr(header->name, '/');

    inode->name = header->name;
    inode->header = header;
    inode->parent = parent;

    if(end != NULL)
    {
        uint64_t index = (uint64_t)end - (uint64_t)header->name;

        char *buffer = NULL;

        if(index == inode->name.size() - 1) //Directory
        {
            char *start = strchr(header->name, '/');

            //Extract last valid '/'
            if(start != end)
            {
                for(;;)
                {
                    char *current = strchr(start + 1, '/');

                    if(current == end)
                    {
                        break;
                    }

                    start = current;
                }

                index = (uint64_t)end - (uint64_t)start - 1;

                buffer = new char[index + 1];

                memcpy(buffer, start + 1, index);
            }
            else //Only one, must be top-level
            {
                index = (uint64_t)end - (uint64_t)header->name;

                buffer = new char[index + 1];

                memcpy(buffer, header->name, index);
            }

            buffer[index] = '\0';

            inode->name = buffer;
        }
        else
        {
            buffer = new char[inode->name.size() - index + 1];

            memcpy(buffer, inode->name.data() + index + 1, inode->name.size() - index);

            buffer[inode->name.size() - index] = '\0';

            inode->name = buffer;
        }

        delete [] buffer;
    }

    if(parent != NULL)
    {
        parent->children.push_back(inode);
    }
    else
    {
        inodes.push_back(inode);
    }
}

string TarFS::ResolveLink(TarHeader *header)
{
    string targetPath = header->name;

    if(header->type == TAR_SYMLINK)
    {
        targetPath = header->link;

        if(targetPath[0] != '/')
        {
            char *newPath = NULL;
            char *ptr = strrchr(header->name, '/');

            if(ptr != NULL)
            {
                int length = (int)(ptr - header->name) + 1;

                newPath = new char[length + strlen(header->link) + 1];

                memcpy(newPath, header->name, length);
                memcpy(newPath + length, header->link, strlen(header->link));

                newPath[length + strlen(header->link)] = '\0';

                targetPath = newPath;

                delete [] newPath;
            }
        }
    }

    return targetPath;
}

void TarFS::ScanInodes(TarFS::Inode *inode)
{
    auto path = ResolveLink(inode->header);

    for(auto &tarHeader : headers)
    {
        if(tarHeader == inode->header)
        {
            continue;
        }

        if(strstr(tarHeader->name, path.data()) == tarHeader->name)
        {
            int found = 0;
            bool invalid = false;

            for(uint32_t i = path.size(); i < strlen(tarHeader->name); i++)
            {
                if(tarHeader->name[i] == '/')
                {
                    found++;

                    if(found > 1)
                    {
                        break;
                    }
                }
                else if(found > 0)
                {
                    invalid = true;

                    break;
                }
            }

            if(found > 1 || invalid)
            {
                continue;
            }

            AddInode(tarHeader, inode);
        }
    }
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
    Inode *inode;

    return FindInode(path, &inode);
}

bool TarFS::FindInode(const char *path, TarFS::Inode **inode)
{
    auto pieces = SplitString(path, '/');
    Inode *target = NULL;
    bool isValid = false;

    for(uint64_t i = 0; i < pieces.size(); i++)
    {
        if(pieces[i].size() == 0) //is a '/'
        {
            isValid = true;

            continue;
        }

        if(pieces[i] == ".")
        {
            isValid = true;

            continue;
        }

        if(pieces[i] == "..")
        {
            if(target != NULL)
            {
                target = target->parent;
            }

            isValid = true;

            continue;
        }

        bool found = false;

        if(i == 0 || target == NULL)
        {
            for(auto &inode : inodes)
            {
                if(pieces[i] == inode->name)
                {
                    found = true;

                    target = inode;

                    break;
                }
            }
        }
        else
        {
            if(target->children.size() == 0)
            {
                ScanInodes(target);
            }

            for(auto &inode : target->children)
            {
                if(pieces[i] == inode->name)
                {
                    found = true;

                    target = inode;

                    break;
                }
            }
        }

        if(!found)
        {
            isValid = false;

            return false;
        }

        isValid = true;
    }

    *inode = target;

    return isValid;
}

struct stat TarFS::Stat(FileSystemHandle handle)
{
    struct stat stat = {0};

    auto file = GetHandle(handle);

    if(file == NULL)
    {
        return stat;
    }

    if(file->header != NULL)
    {
        stat.st_uid = OctToInt(file->header->uid);
        stat.st_gid = OctToInt(file->header->gid);
        stat.st_ino = GetHeaderIndex(file->header);
        stat.st_atim.tv_sec = stat.st_mtim.tv_sec = stat.st_ctim.tv_sec = OctToInt(file->header->mtime);

        switch(file->header->type)
        {
            case TAR_SYMLINK:

                stat.st_mode = S_IFLNK | S_IRWXU;

                break;

            case TAR_FILE:

                stat.st_mode = S_IFMT | S_IRWXU;

                break;

            case TAR_DIRECTORY:

                stat.st_mode = S_IFDIR | S_IRWXU;

                break;
        }
    }
    else //Virtual root
    {
        stat.st_mode = S_IFDIR | S_IRWXU;
    }

    stat.st_nlink = 1;
    stat.st_size = file->length;
    stat.st_blksize = 512;
    stat.st_blocks = stat.st_size / stat.st_blksize + 1;

    return stat;
}

string TarFS::FileLink(FileSystemHandle handle)
{
    auto file = GetHandle(handle);

    if(file == NULL || file->header->type != TAR_SYMLINK)
    {
        return "";
    }

    return file->header->link;
}

FileSystemHandle TarFS::GetFileHandle(const char *path)
{
    Inode *inode = NULL;
    bool found = FindInode(path, &inode);

    if(inode == NULL)
    {
        if(found || strlen(path) == 0) //Virtual Root Dir
        {
            FileHandleData data;

            data.ID = ++fileHandleCounter;
            data.header = NULL;
            data.length = 0;
            data.currentEntry = 0;

            dirent current = {0};

            current.d_ino = 0;
            current.d_reclen = sizeof(dirent);
            strcpy(current.d_name, ".");
            current.d_type = DT_DIR;

            data.entries.push_back(current);

            dirent previous = {0};

            previous.d_ino = 0;
            previous.d_reclen = sizeof(dirent);
            strcpy(previous.d_name, "..");
            previous.d_type = DT_DIR;

            data.entries.push_back(previous);

            for(auto &child : inodes)
            {
                auto childHeader = child->header;
                dirent entry = {0};

                entry.d_ino = GetHeaderIndex(childHeader) + 1;
                entry.d_reclen = sizeof(dirent);

                memcpy(entry.d_name, child->name.data(), child->name.size());

                entry.d_name[child->name.size()] = '\0';

                switch(childHeader->type)
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

            fileHandles.push_back(data);

            return data.ID;
        }

        return 0;
    }

    auto header = inode->header;

    FileHandleData data;

    data.ID = ++fileHandleCounter;
    data.header = inode->header;
    data.length = OctToInt(inode->header->size);

    if(header->type == TAR_DIRECTORY)
    {
        data.currentEntry = 0;

        dirent current = {0};

        current.d_ino = GetHeaderIndex(header) + 1;
        current.d_reclen = sizeof(dirent);
        strcpy(current.d_name, ".");
        current.d_type = DT_DIR;

        data.entries.push_back(current);

        if(strlen(header->name) > 1)
        {
            char *temp = new char[strlen(header->name)];

            memcpy(temp, header->name, strlen(header->name) - 1);

            temp[strlen(header->name) - 1] = '\0';

            char *ptr = strrchr(temp, '/');

            if(ptr != NULL)
            {
                temp[(uint64_t)ptr - (uint64_t)temp + 1] = '\0';

                for(auto &previousHeader : headers)
                {
                    if(previousHeader != header && strcmp(previousHeader->name, temp) == 0)
                    {
                        //DEBUG_OUT("Found previous header %s for directory %s", previousHeader->name, header->name);

                        dirent previous = {0};

                        previous.d_ino = GetHeaderIndex(previousHeader) + 1;
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

        for(auto &child : inode->children)
        {
            auto childHeader = child->header;
            dirent entry = {0};

            entry.d_ino = GetHeaderIndex(childHeader) + 1;
            entry.d_reclen = sizeof(dirent);

            memcpy(entry.d_name, child->name.data(), child->name.size());

            entry.d_name[child->name.size()] = '\0';

            switch(childHeader->type)
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

    fileHandles.push_back(data);

    return data.ID;
}

void TarFS::DisposeFileHandle(FileSystemHandle handle)
{
    //TODO
}

int TarFS::FileHandleType(FileSystemHandle handle)
{
    auto file = GetHandle(handle);

    if(file == NULL)
    {
        return FILE_HANDLE_UNKNOWN;
    }

    if(file->header == NULL)
    {
        return FILE_HANDLE_DIRECTORY;
    }

    switch(file->header->type)
    {
        case TAR_DIRECTORY:

            return FILE_HANDLE_DIRECTORY;

        case TAR_FILE:

            return FILE_HANDLE_FILE;

        case TAR_SYMLINK:

            return FILE_HANDLE_SYMLINK;
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
        dirent *entry = &file->entries[file->currentEntry++];

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

    if(file == NULL || file->header == NULL)
    {
        return 0;
    }

    if(file->header->type == TAR_SYMLINK)
    {
        auto path = ResolveLink(file->header);

        for(auto &header : headers)
        {
            if(path == header->name)
            {
                return OctToInt(header->size);
            }
        }
    }

    return file->length;
}

uint64_t TarFS::ReadFile(FileSystemHandle handle, void *buffer, uint64_t cursor, uint64_t size)
{
    auto file = GetHandle(handle);

    if(file == NULL || file->header == NULL)
    {
        return 0;
    }

    auto fileHeader = file->header;
    uint64_t length = file->length;

    if(fileHeader->type == TAR_SYMLINK)
    {
        auto path = ResolveLink(file->header);
        bool found = false;

        for(auto &header : headers)
        {
            if(path == header->name)
            {
                found = true;
                fileHeader = header;
                length = OctToInt(header->size);

                break;
            }
        }

        if(!found)
        {
            return 0;
        }
    }

    if(cursor > length)
    {
        return 0;
    }

    if(size > length)
    {
        size = length;
    }

    if(cursor + size > length)
    {
        size = length - cursor;
    }

    memcpy(buffer, (uint8_t *)fileHeader + 512 + cursor, size);

    return size;
}

uint64_t TarFS::WriteFile(FileSystemHandle handle, const void *buffer, uint64_t cursor, uint64_t size)
{
    return 0;
}

void TarFS::ListSubdirs(Inode *inode, uint32_t indentation)
{
    for(uint32_t i = 0; i < indentation; i++)
    {
        printf("\t");
    }

    if(inode->children.size() == 0)
    {
        ScanInodes(inode);
    }

    printf("%s (%llu - %i)\n", inode->name.data(), OctToInt(inode->header->size), inode->header->type);

    for(auto &child : inode->children)
    {
        ListSubdirs(child, indentation + 1);
    }
}

void TarFS::DebugListDirectories()
{
    printf("Listing directories\n");

    for(auto &inode : inodes)
    {
        ListSubdirs(inode, 0);
    }

    printf("Listed directories\n");
}

const char *TarFS::VolumeName()
{
    return "tarfs";
}

#include "tarfs.hpp"
#include "frg/string.hpp"
#include "string/stringutils.hpp"
#include "paging/PageFrameAllocator.hpp"
#include "Panic.hpp"

uint64_t OctToInt(const char *str)
{
    uint64_t size = 0;

    for(uint64_t j = 11, count = 1; j > 0; j--, count *= 8)
    {
        size += ((str[j - 1] - '0') * count);
    }

    return size;
}

string TarFS::Inode::FullPath()
{
    string targetPath = name;

    Inode *p = parent;

    while(p != nullptr && p->name.size() != 0)
    {
        targetPath = p->name + "/" + targetPath;

        p = p->parent;
    }

    return targetPath;
}

vector<dirent> TarFS::Inode::GetEntries()
{
    vector<dirent> entries;

    dirent current = {0};

    current.d_ino = 0;
    current.d_reclen = sizeof(dirent);
    strcpy(current.d_name, ".");
    current.d_type = DT_DIR;

    entries.push_back(current);

    strcpy(current.d_name, "..");

    entries.push_back(current);

    for(auto &child : children)
    {
        dirent entry = {0};

        entry.d_ino = child->ID;
        entry.d_reclen = sizeof(dirent);

        memcpy(entry.d_name, child->name.data(), child->name.size());

        entry.d_name[child->name.size()] = '\0';

        switch(child->type)
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

        entries.push_back(entry);
    }

    return entries;
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

    DEBUG_OUT("tarfs: Found %llu headers", headers.size());

    root = new Inode();

    root->isHeader = false;
    root->name = "";
    root->type = TAR_DIRECTORY;

    for(auto &header : headers)
    {
        if(!strstr(header->name, "/"))
        {
            Inode *inode = AddInode(header, root);

            ScanInodes(inode);
        }
    }
}

TarFS::Inode *TarFS::AddInode(TarHeader *header, TarFS::Inode *parent)
{
    auto inode = new Inode();

    auto end = strrchr(header->name, '/');

    inode->name = header->name;
    inode->parent = parent;
    inode->type = header->type;
    inode->header = header;
    inode->isHeader = true;
    inode->gid = OctToInt(header->gid);
    inode->uid = OctToInt(header->uid);
    inode->atime.tv_sec = OctToInt(header->mtime);
    inode->mtime.tv_sec = OctToInt(header->mtime);
    inode->ctime.tv_sec = OctToInt(header->mtime);
    inode->ID = ++inodeCounter;

    if(header->type == TAR_SYMLINK)
    {
        inode->link = header->link;
    }

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

    return inode;
}

string TarFS::ResolveLink(TarFS::Inode *inode)
{
    string targetPath = inode->FullPath();

    if(inode->type == TAR_SYMLINK)
    {
        string linkPath = inode->link;

        if(linkPath[0] != '/')
        {
            char *newPath = NULL;
            char *ptr = strrchr(targetPath.data(), '/');

            if(ptr != NULL)
            {
                int length = (int)(ptr - targetPath.data()) + 1;

                newPath = new char[length + linkPath.size() + 1];

                memcpy(newPath, targetPath.data(), length);
                memcpy(newPath + length, linkPath.data(), linkPath.size());

                newPath[length + linkPath.size()] = '\0';

                targetPath = newPath;

                delete [] newPath;

                return targetPath;
            }
        }
    }

    return targetPath;
}

void TarFS::ScanInodes(TarFS::Inode *inode)
{
    auto path = inode->FullPath();

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

            for(uint32_t i = path.size() + 1; i < strlen(tarHeader->name); i++)
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

            Inode *child = AddInode(tarHeader, inode);

            ScanInodes(child);
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
    Inode *target = root;
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
            if(target != nullptr && target->parent != nullptr)
            {
                target = target->parent;
            }

            isValid = true;

            continue;
        }

        bool found = false;

        for(auto &inode : target->children)
        {
            if(pieces[i] == inode->name)
            {
                found = true;

                target = inode;

                break;
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

    if(file->inode != nullptr)
    {
        stat.st_uid = file->inode->uid;
        stat.st_gid = file->inode->gid;
        stat.st_ino = file->inode->ID;
        stat.st_atim = file->inode->atime;
        stat.st_mtim = file->inode->mtime;
        stat.st_ctim = file->inode->ctime;

        switch(file->inode->type)
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
    stat.st_size = file->inode != nullptr ? (file->inode->isHeader ? OctToInt(file->inode->header->size) : file->inode->data.size()) : 0;
    stat.st_blksize = 512;
    stat.st_blocks = stat.st_size / stat.st_blksize + 1;

    return stat;
}

string TarFS::FileLink(FileSystemHandle handle)
{
    auto file = GetHandle(handle);

    if(file == NULL || file->inode->type != TAR_SYMLINK)
    {
        return "";
    }

    return file->inode->link;
}

FileSystemHandle TarFS::GetFileHandle(const char *path, uint32_t flags)
{
    Inode *inode = NULL;
    bool found = FindInode(path, &inode);

    bool created = false;

    if(inode == NULL)
    {
        if(found || strlen(path) == 0)
        {
            FileHandleData data;
            data.ID = ++fileHandleCounter;
            data.inode = root;
            data.entries = root->GetEntries();

            fileHandles.push_back(data);

            return data.ID;
        }
        else
        {
            if((flags & O_CREAT) == O_CREAT)
            {
                string temp = path;
                string name;

                char *p = strrchr(temp.data(), '/');

                Inode *parent = nullptr;

                if(p == nullptr)
                {
                    parent = root;
                    name = path;
                }
                else
                {
                    *p = '\0';

                    FindInode(temp.data(), &parent);

                    if(parent != nullptr)
                    {
                        name = p + 1;
                    }
                }

                if(parent != nullptr)
                {
                    created = true;

                    inode = new Inode();

                    inode->ID = ++inodeCounter;
                    inode->type = TAR_FILE;
                    inode->parent = parent;
                    inode->name = name;

                    parent->children.push_back(inode);

                    for(auto &handle : fileHandles)
                    {
                        if(handle.inode == parent)
                        {
                            handle.entries = parent->GetEntries();
                        }
                    }
                }
            }
            else
            {
                return 0;
            }
        }
    }

    if(inode == nullptr ||
        ((flags & O_DIRECTORY) == O_DIRECTORY && inode->type != TAR_DIRECTORY) ||
        ((flags & O_CREAT) == O_CREAT && (flags & O_EXCL) == O_EXCL && created == false) ||
        ((flags & O_NOFOLLOW) == O_NOFOLLOW && inode->type == TAR_SYMLINK))
    {
        return 0;
    }

    FileHandleData data;

    data.ID = ++fileHandleCounter;
    data.inode = inode;
    data.flags = flags;

    if(inode->type == TAR_DIRECTORY)
    {
        data.currentEntry = 0;

        data.entries = inode->GetEntries();
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

    if(file == NULL || file->inode == NULL)
    {
        return FILE_HANDLE_UNKNOWN;
    }

    switch(file->inode->type)
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

    if(file->currentEntry < (int)file->entries.size())
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

    if(file == NULL || file->inode == NULL)
    {
        return 0;
    }

    if(file->inode->type == TAR_SYMLINK)
    {
        auto path = ResolveLink(file->inode);

        Inode *link = nullptr;
        
        if(FindInode(path.data(), &link))
        {
            return link->isHeader ? OctToInt(link->header->size) : link->data.size();
        }
    }

    return file->inode->isHeader ? OctToInt(file->inode->header->size) : file->inode->data.size();
}

uint64_t TarFS::ReadFile(FileSystemHandle handle, void *buffer, uint64_t cursor, uint64_t size)
{
    auto file = GetHandle(handle);

    if(file == NULL ||
        file->inode == NULL ||
        (file->flags & O_WRONLY) == O_WRONLY ||
        (file->flags & O_PATH) == O_PATH)
    {
        return 0;
    }

    uint64_t length = file->inode->isHeader ? OctToInt(file->inode->header->size) : file->inode->data.size();

    Inode *inode = file->inode;

    if(file->inode->type == TAR_SYMLINK)
    {
        auto path = ResolveLink(file->inode);

        Inode *link = nullptr;

        bool found = FindInode(path.data(), &link);

        if(found)
        {
            length = link->isHeader ? OctToInt(link->header->size) : link->data.size();
            inode = link;
        }
        else
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

    memcpy(buffer, (inode->isHeader ? (uint8_t *)inode->header + 512 : inode->data.data()) + cursor, size);

    return size;
}

uint64_t TarFS::WriteFile(FileSystemHandle handle, const void *buffer, uint64_t cursor, uint64_t size)
{
    auto file = GetHandle(handle);

    if(file == NULL ||
        file->inode == NULL ||
        file->inode->type != TAR_FILE ||
        (file->flags & O_RDONLY) == O_RDONLY ||
        (file->flags & O_PATH) == O_PATH)
    {
        return 0;
    }

    if(file->inode->isHeader)
    {
        file->inode->isHeader = false;
    }

    if(file->flags & O_APPEND)
    {
        cursor = file->inode->data.size();
    }

    if(cursor + size > file->inode->data.size())
    {
        file->inode->data.resize(cursor + size);
    }

    memcpy(file->inode->data.data() + cursor, buffer, size);

    return size;
}

void TarFS::ListSubdirs(Inode *inode, uint32_t indentation)
{
    for(uint32_t i = 0; i < indentation; i++)
    {
        printf("\t");
    }

    printf("%s (%llu - %i)\n", inode->name.data(), inode->isHeader ? OctToInt(inode->header->size) : inode->data.size(), inode->header->type);

    for(auto &child : inode->children)
    {
        ListSubdirs(child, indentation + 1);
    }
}

void TarFS::DebugListDirectories()
{
    printf("Listing directories\n");

    for(auto &inode : root->children)
    {
        ListSubdirs(inode, 0);
    }

    printf("Listed directories\n");
}

const char *TarFS::VolumeName()
{
    return "tarfs";
}

bool TarFS::MakeDir(const char *path, mode_t mode)
{
    Inode *inode = nullptr;

    if(FindInode(path, &inode))
    {
        return false;
    }

    string temp = path;
    string name;

    char *p = strrchr(temp.data(), '/');

    //Check for end `/`
    if(p == temp.data() + (temp.size() - 1))
    {
        *p = '\0';

        p = strrchr(temp.data(), '/');
    }

    Inode *parent = nullptr;

    if(p == nullptr)
    {
        parent = root;
        name = path;
    }
    else
    {
        *p = 0;

        if(FindInode(temp.data(), &inode))
        {
            parent = inode;
        }

        name = p + 1;
    }

    if(parent != nullptr)
    {
        inode = new Inode();
        inode->ID = ++inodeCounter;
        inode->type = TAR_DIRECTORY;
        inode->name = name;
        inode->parent = parent;

        parent->children.push_back(inode);

        for(auto &child : fileHandles)
        {
            if(child.inode == parent)
            {
                child.entries = parent->GetEntries();
            }
        }

        return true;
    }

    return false;
}

bool TarFS::Rename(const char *path, const char *newPath)
{
    Inode *inode;

    if(FindInode(path, &inode) == false)
    {
        return false;
    }

    char *p = (char *)strrchr(newPath, '/');
    char *p2 = (char *)strrchr(path, '/');

    if(p == nullptr || p2 == nullptr)
    {
        return false;
    }

    if(p != nullptr)
    {
        *p = '\0';
    }

    if(p2 != nullptr)
    {
        *p2 = '\0';
    }

    Inode *a = nullptr, *b = nullptr;

    if(FindInode(p, &a) == false || FindInode(p2, &b) == false || a != b)
    {
        *p = '/';
        *p2 = '/';

        return false;
    }

    *p = '/';
    *p2 = '/';

    inode->name = p + 1;

    return true;
}

bool TarFS::RemoveDir(const char *path)
{
    Inode *inode;

    if(FindInode(path, &inode) == false)
    {
        return false;
    }

    RemoveInode(inode);

    return true;
}

void TarFS::RemoveInode(Inode *inode)
{
    for(auto &child : inode->children)
    {
        RemoveInode(child);

        delete child;
    }

    if(inode->parent != nullptr)
    {
        Inode *parent = inode->parent;

        vector<Inode *> children;

        for(auto &child : parent->children)
        {
            if(child == inode)
            {
                continue;
            }

            children.push_back(child);
        }

        parent->children = children;
    }

    delete inode;
}

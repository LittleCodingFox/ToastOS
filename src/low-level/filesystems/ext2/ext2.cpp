#include "ext2.hpp"
#include "timer/Timer.hpp"

#define FORMAT_MASK                     0xF000

#define EXT2_FS_UNRECOVERABLE_ERRORS    3

#define EXT2_IF_COMPRESSION             0x01
#define EXT2_IF_EXTENTS                 0x40
#define EXT2_IF_64BIT                   0x80
#define EXT2_IF_INLINE_DATA             0x8000
#define EXT2_IF_ENCRYPT                 0x10000
#define EXT2_FEATURE_INCOMPAT_META_BG   0x0010

#define EXT2_S_MAGIC                    0xEF53

#define EXT2_INO_DIRECTORY              0x4000

#define EXT2_FILE_TYPE_UNKNOWN          0
#define EXT2_FILE_TYPE_FILE             1
#define EXT2_FILE_TYPE_DIRECTORY        2
#define EXT2_FILE_TYPE_CHAR_DEVICE      3
#define EXT2_FILE_TYPE_BLOCK_DEVICE     4
#define EXT2_FILE_TYPE_FIFO             5
#define EXT2_FILE_TYPE_SOCKET           6
#define EXT2_FILE_TYPE_SYMLINK          7

struct PACKED Ext2BlockGroupDescriptor
{
    uint32_t blockBitmap;
    uint32_t inodeBitmap;
    uint32_t inodeTable;

    uint16_t freeBlocksCount;
    uint16_t freeInodesCount;
    uint16_t dirCount;

    uint16_t reserved[7];
};

struct PACKED Ext2ExtentIndex
{
    uint32_t block;
    uint32_t leaf;
    uint16_t leafHigh;
    uint16_t empty;
};

void Ext2FileSystem::Initialize(uint64_t sector, uint64_t sectorCount)
{
    if(!partition->Read(&superblock, 2, 2))
    {
        printf("ext2: Failed to read superblock\n");

        return;
    }

    if(superblock.revLevel != 0 &&
        ((superblock.featureIncompat & EXT2_IF_COMPRESSION) ||
        (superblock.featureIncompat & EXT2_IF_INLINE_DATA) ||
        (superblock.featureIncompat & EXT2_FEATURE_INCOMPAT_META_BG) ||
        (superblock.featureIncompat & EXT2_IF_ENCRYPT)))
    {
        printf("ext2: Unsupported partition\n");

        return;
    }

    if(superblock.state == EXT2_FS_UNRECOVERABLE_ERRORS)
    {
        printf("ext2: Partition has unrecoverable errors\n");

        return;
    }

    blockSize = (uint64_t)1024 << superblock.logBlockSize;

    if(!FindInode(&root, 2))
    {
        printf("ext2: Failed to find root node\n");

        return;
    }

    superblock.volumeName[15] = '\0';

    printf("ext2: Initialized\n");

    initOK = true;
}

bool Ext2FileSystem::FindInode(Ext2Inode *result, uint64_t inode)
{
    if(inode == 0)
    {
        return false;
    }

    uint64_t inodeBlockGroup = (inode - 1) / superblock.inodesPerGroup;
    uint64_t inodeTableIndex = (inode - 1) % superblock.inodesPerGroup;
    
    uint64_t blockGroupDescriptorStartOffset = blockSize >= 2048 ? blockSize : blockSize * 2;
    uint64_t inodeSize = superblock.revLevel == 0 ? sizeof(Ext2Inode) : superblock.inodeSize;
    uint64_t inodeOffset;

    uint64_t blockGroupDescriptorOffset = blockGroupDescriptorStartOffset + sizeof(Ext2BlockGroupDescriptor) * inodeBlockGroup;

    Ext2BlockGroupDescriptor targetDescriptor;

    if(!partition->ReadUnaligned(&targetDescriptor, blockGroupDescriptorOffset, sizeof(Ext2BlockGroupDescriptor)))
    {
        return false;
    }

    inodeOffset = targetDescriptor.inodeTable * blockSize + inodeSize * inodeTableIndex;

    return partition->ReadUnaligned(result, inodeOffset, sizeof(Ext2Inode));
}

vector<uint32_t> Ext2FileSystem::CreateAllocMap(Ext2Inode *inode)
{
    size_t entriesPerBlock = blockSize / sizeof(uint32_t);

    vector<uint32_t> allocMap;

    allocMap.resize(inode->blockCount);

    for(uint32_t i = 0; i < inode->blockCount; i++)
    {
        uint32_t block = i;

        if(block < 12)
        {
            allocMap[i] = inode->blocks[block];
        }
        else
        {
            block -= 12;

            if(block >= entriesPerBlock)
            {
                block -= entriesPerBlock;

                uint32_t index = block / entriesPerBlock;
                uint32_t indirectBlock;

                if(index >= entriesPerBlock)
                {
                    uint32_t firstIndex = index / entriesPerBlock;
                    uint32_t firstIndirectBlock;

                    partition->ReadUnaligned(&firstIndirectBlock,
                        inode->blocks[14] * blockSize + firstIndex * sizeof(uint32_t),
                        sizeof(uint32_t));

                    uint32_t secondIndex = index % entriesPerBlock;

                    partition->ReadUnaligned(&indirectBlock,
                        firstIndirectBlock * blockSize + secondIndex * sizeof(uint32_t),
                        sizeof(uint32_t));
                }
                else
                {
                    partition->ReadUnaligned(&indirectBlock,
                        inode->blocks[13] * blockSize + index * sizeof(uint32_t),
                        sizeof(uint32_t));
                }

                for(uint32_t j = 0; j < entriesPerBlock; j++)
                {
                    if(i + j >= inode->blockCount)
                    {
                        return allocMap;
                    }

                    partition->ReadUnaligned(&allocMap[i + j],
                        indirectBlock * blockSize + j * sizeof(uint32_t),
                        sizeof(uint32_t));
                }

                i += entriesPerBlock - 1;
            }
            else
            {
                partition->ReadUnaligned(&allocMap[i],
                    inode->blocks[12] * blockSize + block * sizeof(uint32_t),
                    sizeof(uint32_t));
            }
        }
    }

    return allocMap;
}

bool Ext2FileSystem::SymlinkToInode(Ext2Inode *inode)
{
    if(inode->size < 59)
    {
        Ext2DirEntry dirent;

        char *symlink = (char *)inode->blocks;

        symlink[59] = 0;

        if(!ParseDirent(&dirent, symlink))
        {
            return false;
        }

        FindInode(inode, dirent.inode);

        return true;
    }

    return false;
}

bool Ext2FileSystem::ReadDirentName(Ext2DirEntry *dirent, const vector<uint32_t> &allocMap, char *buffer, uint64_t offset)
{
    memset(buffer, 0, dirent->nameLength + 1);

    return ReadInode(buffer, offset + sizeof(Ext2DirEntry), dirent->nameLength, allocMap);
}

bool Ext2FileSystem::ParseDirent(Ext2DirEntry *dirent, const char *path)
{
    if(*path == '/')
    {
        path++;
    }

    Ext2Inode currentInode = root;

    bool escape = false;

    static char token[256];

next:
    memset(token, 0, 256);

    for(size_t i = 0; i < 255 && *path != '/' && *path != '\0'; i++, path++)
    {
        token[i] = *path;
    }

    if(*path == '\0')
    {
        escape = true;
    }
    else
    {
        path++;
    }

    vector<uint32_t> allocMap = CreateAllocMap(&currentInode);

    for(uint32_t i = 0; i < currentInode.size; )
    {
        if(!ReadInode(dirent, i, sizeof(Ext2DirEntry), allocMap))
        {
            return false;            
        }

        char name[dirent->nameLength + 1];

        if(!ReadDirentName(dirent, allocMap, name, i))
        {
            return false;
        }

        int test = strcmp(token, name);

        if(test == 0)
        {
            if(escape)
            {
                return true;
            }
            else
            {
                if(!FindInode(&currentInode, dirent->inode))
                {
                    return false;
                }

                while((currentInode.mode & FORMAT_MASK) != S_IFDIR)
                {
                    if((currentInode.mode & FORMAT_MASK) == S_IFLNK)
                    {
                        if(!SymlinkToInode(&currentInode))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        return false;
                    }
                }

                allocMap = CreateAllocMap(&currentInode);

                goto next;
            }
        }

        i += dirent->recordLength;
    }

    goto out;

out:
    return false;
}

bool Ext2FileSystem::ReadDirent(Ext2DirEntry *dirent, const vector<uint32_t> &allocMap, uint64_t offset)
{
    return ReadInode(dirent, offset, sizeof(Ext2DirEntry), allocMap);
}

void Ext2FileSystem::ListSubdirs(Ext2Inode *inode, const vector<uint32_t> &allocMap, uint32_t indentation)
{
    Ext2DirEntry dirent;
    int index = 0;

    for(uint32_t i = 0; i < inode->size;)
    {
        if(!ReadDirent(&dirent, allocMap, i))
        {
            printf("ext2: Failed to read dirent\n");

            return;
        }

        for(uint32_t j = 0; j < indentation; j++)
        {
            printf("\t");
        }

        char name[dirent.nameLength + 1];

        if(!ReadDirentName(&dirent, allocMap, name, i))
        {
            printf("ext2: Failed to read dirent name\n");

            return;
        }

        printf("%x %s %x\n", index++, name, dirent.type);

        if(dirent.type == EXT2_FILE_TYPE_DIRECTORY && index >= 3)
        {
            Ext2Inode dir;

            if(FindInode(&dir, dirent.inode))
            {
                auto dirAlloc = CreateAllocMap(&dir);

                ListSubdirs(&dir, dirAlloc, indentation + 1);
            }
        }

        i += dirent.recordLength;
    }
}

void Ext2FileSystem::DebugListDirectories()
{
    if(!initOK)
    {
        return;
    }

    auto allocMap = CreateAllocMap(&root);

    ListSubdirs(&root, allocMap, 0);
}

bool Ext2FileSystem::Exists(const char *path)
{
    if(!initOK)
    {
        return false;
    }

    Ext2DirEntry dirent;

    if(!ParseDirent(&dirent, path))
    {
        return false;
    }

    Ext2Inode inode;

    if(!FindInode(&inode, dirent.inode))
    {
        return false;
    }

    return true;
}

Ext2FileSystem::FileHandleData *Ext2FileSystem::GetHandle(FileSystemHandle handle)
{
    if(!initOK)
    {
        return NULL;
    }

    for(auto &fileHandle : fileHandles)
    {
        if(fileHandle.ID == handle)
        {
            return &fileHandle;
        }
    }

    return NULL;
}

FileSystemHandle Ext2FileSystem::GetFileHandle(const char *path)
{
    if(!initOK)
    {
        return 0;
    }

    Ext2DirEntry dirent;

    if(!ParseDirent(&dirent, path))
    {
        return 0;
    }

    Ext2Inode inode;

    if(!FindInode(&inode, dirent.inode))
    {
        return 0;
    }

    FileHandleData data;
    data.ID = ++fileHandleCounter;
    data.inode = inode;
    data.allocMap = CreateAllocMap(&data.inode);
    data.length = inode.size;

    fileHandles.push_back(data);

    return data.ID;
}

int Ext2FileSystem::FileHandleType(FileSystemHandle handle)
{
    if(!initOK)
    {
        return FILE_HANDLE_UNKNOWN;
    }

    auto fileHandle = GetHandle(handle);

    if(fileHandle == NULL)
    {
        return FILE_HANDLE_UNKNOWN;
    }

    switch(fileHandle->inode.mode & FORMAT_MASK)
    {
        case S_IFCHR:
            return FILE_HANDLE_CHARDEVICE;

        case S_IFDIR:
            return FILE_HANDLE_DIRECTORY;

        case S_IFLNK:
            return FILE_HANDLE_SYMLINK;

        case S_IFREG:
            return FILE_HANDLE_FILE;

        default:
            return FILE_HANDLE_UNKNOWN;
    }
}

bool Ext2FileSystem::ReadInode(void *buffer, uint64_t position, uint64_t count, const vector<uint32_t> &allocMap)
{
    for(uint64_t progress = 0; progress < count;)
    {
        uint64_t block = (position + progress) / blockSize;
        uint64_t chunk = count - progress;

        uint64_t offset = (position + progress) % blockSize;

        if(chunk > blockSize - offset)
        {
            chunk = blockSize - offset;
        }

        uint32_t blockIndex = allocMap[block];

        if(!partition->ReadUnaligned((uint8_t *)buffer + progress, blockIndex * blockSize + offset, chunk))
        {
            return false;
        }

        progress += chunk;
    }

    return true;
}

bool Ext2FileSystem::WriteInode(const void *buffer, uint64_t position, uint64_t count, const vector<uint32_t> &allocMap)
{
    for(uint64_t progress = 0; progress < count;)
    {
        uint64_t block = (position + progress) / blockSize;
        uint64_t chunk = count - progress;

        uint64_t offset = (position + progress) % blockSize;

        if(chunk > blockSize - offset)
        {
            chunk = blockSize - offset;
        }

        uint32_t blockIndex = allocMap[block];

        if(!partition->WriteUnaligned((uint8_t *)buffer + progress, blockIndex * blockSize + offset, chunk))
        {
            return false;
        }

        progress += chunk;
    }

    return true;
}

bool Ext2FileSystem::IsValid(GPT::Partition *partition)
{
    Ext2Superblock superblock;

    return partition->Read(&superblock, 2, 2) && superblock.magic == EXT2_S_MAGIC;
}

void Ext2FileSystem::DisposeFileHandle(FileSystemHandle handle)
{
    //TODO
}

uint64_t Ext2FileSystem::FileLength(FileSystemHandle handle)
{
    if(!initOK)
    {
        return 0;
    }

    auto fileHandle = GetHandle(handle);

    if(fileHandle == NULL)
    {
        return 0;
    }

    return fileHandle->length;
}

string Ext2FileSystem::FileLink(FileSystemHandle handle)
{
    if(!initOK)
    {
        return "";
    }

    auto fileHandle = GetHandle(handle);

    if(fileHandle == NULL)
    {
        return "";
    }

    if((fileHandle->inode.mode & FORMAT_MASK) != S_IFLNK)
    {
        return "";
    }

    char *symlink = (char *)fileHandle->inode.blocks;

    symlink[59] = 0;

    return symlink;
}

uint64_t Ext2FileSystem::ReadFile(FileSystemHandle handle, void *buffer, uint64_t cursor, uint64_t size)
{
    if(!initOK)
    {
        return 0;
    }

    auto fileHandle = GetHandle(handle);

    if(fileHandle == NULL)
    {
        return 0;
    }

    switch(fileHandle->inode.mode & FORMAT_MASK)
    {
        case S_IFREG:
            if(!ReadInode(buffer, cursor, size, fileHandle->allocMap))
            {
                return 0;
            }

            return size;

        default:
            return 0;
    }
}

uint64_t Ext2FileSystem::WriteFile(FileSystemHandle handle, const void *buffer, uint64_t cursor, uint64_t size)
{
    return 0;
}

const char *Ext2FileSystem::VolumeName()
{
    if(initOK == false)
    {
        return "";
    }

    return (char *)superblock.volumeName;
}

struct stat Ext2FileSystem::Stat(FileSystemHandle handle)
{
    if(!initOK)
    {
        return {};
    }

    auto fileHandle = GetHandle(handle);

    if(fileHandle == NULL)
    {
        return {};
    }

    struct stat outValue;

    outValue.st_atim.tv_sec = fileHandle->inode.atime;
    outValue.st_blksize = blockSize;
    outValue.st_blocks = fileHandle->inode.blockCount;
    outValue.st_ctim.tv_sec = fileHandle->inode.ctime;
    outValue.st_dev = 0;
    outValue.st_gid = fileHandle->inode.GID;
    outValue.st_ino = 0; //TODO
    outValue.st_mode = fileHandle->inode.mode;
    outValue.st_mtim.tv_sec = fileHandle->inode.mtime;
    outValue.st_nlink = fileHandle->inode.linkCount;
    outValue.st_rdev = 0;
    outValue.st_size = fileHandle->inode.size;
    outValue.st_uid = fileHandle->inode.UID;

    return outValue;
}

dirent *Ext2FileSystem::ReadEntries(FileSystemHandle handle)
{
    if(!initOK)
    {
        return NULL;
    }

    auto fileHandle = GetHandle(handle);

    if(fileHandle == NULL)
    {
        return NULL;
    }

    Ext2DirEntry dir;
    int index = 0;

    for(uint32_t i = 0; i < fileHandle->inode.size; index++)
    {
        if(!ReadDirent(&dir, fileHandle->allocMap, i))
        {
            return NULL;
        }

        char name[dir.nameLength + 1];

        if(!ReadDirentName(&dir, fileHandle->allocMap, name, i))
        {
            return NULL;
        }

        if(index == fileHandle->currentEntry)
        {
            fileHandle->dir.d_ino = 0;
            fileHandle->dir.d_off = i;
            fileHandle->dir.d_reclen = sizeof(dirent);

            switch(fileHandle->inode.mode & FORMAT_MASK)
            {
                case S_IFBLK:

                    fileHandle->dir.d_type = DT_BLK;

                    break;

                case S_IFCHR:

                    fileHandle->dir.d_type = DT_CHR;

                    break;

                case S_IFDIR:

                    fileHandle->dir.d_type = DT_DIR;

                    break;

                case S_IFIFO:

                    fileHandle->dir.d_type = DT_FIFO;

                    break;

                case S_IFLNK:

                    fileHandle->dir.d_type = DT_LNK;

                    break;

                case S_IFREG:

                    fileHandle->dir.d_type = DT_REG;

                    break;

                case S_IFSOCK:

                    fileHandle->dir.d_type = DT_SOCK;

                    break;
            }

            auto size = dir.nameLength + 1;

            if(size > sizeof(fileHandle->dir.d_name))
            {
                size = sizeof(fileHandle->dir.d_name);
            }

            memcpy(fileHandle->dir.d_name, name, size);

            fileHandle->currentEntry++;

            return &fileHandle->dir;
        }

        i += dir.recordLength;
    }

    return NULL;
}

void Ext2FileSystem::CloseDir(FileSystemHandle handle)
{
    if(!initOK)
    {
        return;
    }

    auto fileHandle = GetHandle(handle);

    if(fileHandle == NULL)
    {
        return;
    }

    fileHandle->currentEntry = 0;
}

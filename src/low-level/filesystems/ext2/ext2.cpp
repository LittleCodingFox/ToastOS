#include "ext2.hpp"
#include "Bitmap.hpp"
#include "stacktrace/stacktrace.hpp"

namespace FileSystem
{
    namespace ext2
    {
        const char *Ext2DirectoryTypeNames[8] =
        {
            "Unknown",
            "Regular File",
            "Directory",
            "Char Device",
            "Block Device",
            "FIFO",
            "Socket",
            "Link",
        };

        Inode::Inode(Inode &other) : ID(other.ID)
        {
            memcpy(&this->inode, &other.inode, sizeof(DiskInode));
        }

        Inode::Inode(const Inode &other) : ID(other.ID)
        {
            memcpy(&this->inode, &other.inode, sizeof(DiskInode));
        }

        bool Ext2FileSystem::GetBlockForInode(uint64_t inode, uint64_t &blockID, uint64_t &offset)
        {
            if(inode != EXT2_ROOT_INO && inode < superBlock.firstInode)
            {
                return false;
            }

            if(inode > superBlock.inodeCount)
            {
                return false;
            }

            DiskGroupDescription groupDescriptor = ReadGroupFromGroupID(GroupFromInode(inode));

            uint64_t fullOffset = ((inode - 1) % inodePerGroup) * superBlock.inodeSize;
            blockID = groupDescriptor.inodeTable + (fullOffset >> superBlock.logBlockSize);
            offset = fullOffset & (blockSize - 1);

            return true;
        }

        bool Ext2FileSystem::FlushSuperblock()
        {
            return partition->WriteUnaligned(&superBlock, 1024, sizeof(superBlock));
        }

        void Ext2FileSystem::ReadBlock(uint64_t blockID, void *buffer)
        {
            partition->Read(buffer, (offset + (blockID * blockSize)) / 512, blockSize / 512);
        }

        void Ext2FileSystem::ReadBlocks(uint64_t blockID, uint64_t length, void *buffer)
        {
            uint8_t *ptr = (uint8_t *)buffer;

            for(uint64_t i = 0; i < length; i++)
            {
                partition->Read(ptr + (blockSize * i), (offset + ((blockID + i) * blockSize)) / 512, blockSize / 512);
            }
        }

        DiskGroupDescription Ext2FileSystem::ReadGroupFromInode(uint64_t inode)
        {
            uint64_t currentBlockGroup = (inode - 1) / superBlock.inodesPerGroup;
            uint64_t blockGroupStart = blockGroupDescriptorTable * blockSize;
            uint64_t groupOffset = blockGroupStart + sizeof(DiskGroupDescription) * currentBlockGroup;

            DiskGroupDescription group;

            partition->ReadUnaligned(&group, offset + groupOffset, sizeof(DiskGroupDescription));

            return group;
        }

        void Ext2FileSystem::WriteGroupFromInode(uint64_t inode, const DiskGroupDescription &group)
        {
            uint64_t currentBlockGroup = (inode - 1) / superBlock.inodesPerGroup;
            uint64_t blockGroupStart = blockGroupDescriptorTable * blockSize;
            uint64_t groupOffset = blockGroupStart + sizeof(DiskGroupDescription) * currentBlockGroup;

            partition->WriteUnaligned(&group, offset + groupOffset, sizeof(DiskGroupDescription));
        }

        DiskGroupDescription Ext2FileSystem::ReadGroupFromGroupID(uint64_t GID)
        {
            uint64_t currentBlockGroup = GID;
            uint64_t blockGroupStart = blockGroupDescriptorTable * blockSize;
            uint64_t groupOffset = blockGroupStart + sizeof(DiskGroupDescription) * currentBlockGroup;

            DiskGroupDescription group;

            partition->ReadUnaligned(&group, offset + groupOffset, sizeof(DiskGroupDescription));

            return group;
        }

        void Ext2FileSystem::WriteGroupFromGroupID(uint64_t GID, const DiskGroupDescription &group)
        {
            uint64_t currentBlockGroup = GID;
            uint64_t blockGroupStart = blockGroupDescriptorTable * blockSize;
            uint64_t groupOffset = blockGroupStart + sizeof(DiskGroupDescription) * currentBlockGroup;

            partition->WriteUnaligned(&group, offset + groupOffset, sizeof(DiskGroupDescription));
        }

        bool Ext2FileSystem::InodeRead(void *buffer, uint64_t cursor, uint64_t count, const Inode &parent)
        {
            for(uint64_t readBytes = 0; readBytes < count;)
            {
                uint64_t currentBlock = (cursor + readBytes) / blockSize;
                uint64_t blockOffset = (cursor + readBytes) % blockSize;

                if(currentBlock > parent.inode.blocks)
                {
                    break;
                }

                uint64_t chunkSize = count - readBytes;

                if(chunkSize > blockSize - blockOffset)
                {
                    chunkSize = blockSize - blockOffset;
                }

                uint64_t block = GetInodeBlockMap(parent, currentBlock);

                if(!partition->ReadUnaligned((uint8_t *)buffer + readBytes, offset + (block * blockSize) + blockOffset, chunkSize))
                {
                    return false;
                }

                readBytes += chunkSize;
            }

            return true;
        }

        bool Ext2FileSystem::InodeWrite(const void *buffer, uint64_t cursor, uint64_t count, const Inode &parent)
        {
            for(uint64_t writtenBytes = 0; writtenBytes < count;)
            {
                uint64_t currentBlock = (cursor + writtenBytes) / blockSize;
                uint64_t blockOffset = (cursor + writtenBytes) % blockSize;

                if(currentBlock > parent.inode.blocks)
                {
                    break;
                }

                uint64_t chunkSize = count - writtenBytes;

                if(chunkSize > blockSize - blockOffset)
                {
                    chunkSize = blockSize - blockOffset;
                }

                uint64_t block = GetInodeBlockMap(parent, currentBlock);

                if(!partition->WriteUnaligned((uint8_t *)buffer + writtenBytes,
                    offset + (block * blockSize) + blockOffset, chunkSize))
                {
                    return false;
                }

                writtenBytes += chunkSize;
            }

            return true;
        }

        uint64_t Ext2FileSystem::GetInodeBlockMap(const Inode &inode, uint64_t blockID)
        {
            if(inode.inode.flags & 0x80000)
            {
                return 0;
            }
            else if(blockID > inode.inode.blocks)
            {
                return 0;
            }

            uint64_t outValue = 0;
            uint64_t entriesPerBlock = blockSize / sizeof(uint64_t);

            if(blockID < 12)
            {
                outValue = inode.inode.blockPointer[blockID];
            }
            else if(blockID - 12 < entriesPerBlock)
            {
                uint32_t nid = blockID - 12;
                partition->ReadUnaligned(&outValue,
                    offset + inode.inode.singleIndirectBlockPointer * blockSize + nid * sizeof(uint32_t),
                    sizeof(uint32_t));
            }
            else if(blockID - 12 / entriesPerBlock < entriesPerBlock)
            {
                uint32_t nid = blockID - (12 + entriesPerBlock);
                uint32_t blockIndex = nid / entriesPerBlock;
                uint32_t indirectBlockID = 0;
                uint32_t subEntry = nid % entriesPerBlock;

                partition->ReadUnaligned(&indirectBlockID,
                    offset + inode.inode.doubleIndirectBlockPointer * blockSize + blockIndex * sizeof(uint32_t),
                    sizeof(uint32_t));

                partition->ReadUnaligned(&outValue,
                    offset + indirectBlockID * blockSize + subEntry * sizeof(uint32_t),
                    sizeof(uint32_t));
            }
            else //TODO: Triple indirection support
            {
                return 0;
            }

            return outValue;
        }

        void Ext2FileSystem::AddInodeBlockMap(Inode &inode, uint32_t blockAddress)
        {
            if(inode.inode.flags & 0x80000)
            {
                return;
            }

            uint64_t entriesPerBlock = blockSize / sizeof(uint32_t);
            uint64_t blockID = inode.inode.blocks - 1;
            inode.inode.blocks++;

            if(blockID < 12)
            {
                inode.inode.blockPointer[blockID] = blockAddress;

                WriteInode(inode);
            }
            else if(blockID - 12 < entriesPerBlock)
            {
                if(blockID - 1 < 12)
                {
                    inode.inode.singleIndirectBlockPointer = AllocBlockForInode(inode);
                }

                uint32_t nid = blockID - 12;
                partition->WriteUnaligned(&blockAddress,
                    offset + inode.inode.singleIndirectBlockPointer * blockSize + nid * sizeof(uint32_t),
                    sizeof(uint32_t));

                WriteInode(inode);
            }
            else if(blockID - 12 / entriesPerBlock < entriesPerBlock)
            {
                if(blockID - 1 < entriesPerBlock)
                {
                    inode.inode.doubleIndirectBlockPointer = AllocBlockForInode(inode);
                }

                uint32_t nid = blockID - 12 + entriesPerBlock;
                uint32_t blockIndex = nid / entriesPerBlock;
                uint32_t indirectBlockID = 0;
                uint32_t subentry = nid % entriesPerBlock;

                partition->ReadUnaligned(&indirectBlockID,
                    offset + inode.inode.doubleIndirectBlockPointer * blockSize + blockIndex * sizeof(uint32_t),
                    sizeof(uint32_t));

                if(indirectBlockID == 0)
                {
                    indirectBlockID = AllocBlockForInode(inode);

                    partition->WriteUnaligned(&indirectBlockID,
                        offset + inode.inode.doubleIndirectBlockPointer * blockSize + blockIndex * sizeof(uint32_t),
                        sizeof(uint32_t));
                }

                partition->WriteUnaligned(&blockAddress,
                    offset + indirectBlockID * blockSize + subentry * sizeof(uint32_t),
                    sizeof(uint32_t));

                WriteInode(inode);
            }
            else
            {
                //TODO: Triple indirect blocks
            }
        }

        uint32_t *Ext2FileSystem::CreateInodeBlockMap(const Inode &inode)
        {
            if(inode.inode.flags & 0x80000)
            {
                return 0;
            }

            uint64_t entriesPerBlock = blockSize / sizeof(uint32_t);
            uint32_t *data = (uint32_t *)malloc((inode.inode.blocks + 2) * sizeof(uint32_t));

            for(uint32_t blockID = 0; blockID < inode.inode.blocks; blockID++)
            {
                if(blockID < 12)
                {
                    data[blockID] = inode.inode.blockPointer[blockID];
                }
                else if(blockID - 12 < entriesPerBlock)
                {
                    uint32_t nid = blockID - 12;

                    partition->ReadUnaligned(data + blockID,
                        offset + inode.inode.singleIndirectBlockPointer * blockSize + nid * sizeof(uint32_t),
                        sizeof(uint32_t));
                }
                else if(blockID - 12 / entriesPerBlock < entriesPerBlock)
                {
                    uint32_t nid = blockID - (12 + entriesPerBlock);
                    uint32_t blockIndex = nid / entriesPerBlock;
                    uint32_t indirectBlockID = 0;

                    partition->ReadUnaligned(&indirectBlockID,
                        offset + inode.inode.doubleIndirectBlockPointer * blockSize + blockIndex * sizeof(uint32_t),
                        sizeof(uint32_t));

                    for(uint32_t subEntry = 0; subEntry < entriesPerBlock; subEntry++)
                    {
                        if(blockID + subEntry > inode.inode.blocks)
                        {
                            return data;
                        }

                        partition->ReadUnaligned(data + blockID + subEntry,
                            offset + indirectBlockID * blockSize + subEntry * sizeof(uint32_t),
                            sizeof(uint32_t));
                    }

                    blockID += entriesPerBlock - 1;
                }
                else //TODO: Triple Indirects
                {
                    free(data);

                    return NULL;
                }
            }

            return data;
        }

        Inode Ext2FileSystem::GetInode(uint64_t inode)
        {
            Inode outValue;

            uint64_t currentBlockGroup = (inode - 1) / superBlock.inodesPerGroup;
            uint64_t insideBlockGroup = (inode - 1) % superBlock.inodesPerGroup;
            uint64_t blockGroupStart = blockGroupDescriptorTable * blockSize;

            DiskGroupDescription group;

            uint64_t groupOffset = blockGroupStart + sizeof(DiskGroupDescription) * currentBlockGroup;

            partition->ReadUnaligned(&group, offset + groupOffset, sizeof(DiskGroupDescription));

            uint64_t inodeOffset = group.inodeTable * blockSize + superBlock.inodeSize * insideBlockGroup;

            partition->ReadUnaligned(&outValue.inode, offset + inodeOffset, sizeof(DiskInode));

            outValue.ID = inode;

            return outValue;
        }

        bool Ext2FileSystem::WriteInode(const Inode &inode)
        {
            uint64_t currentBlockGroup = (inode.ID - 1) / superBlock.inodesPerGroup;
            uint64_t insideBlockGroup = (inode.ID - 1) % superBlock.inodesPerGroup;
            uint64_t blockGroupStart = blockGroupDescriptorTable * blockSize;

            DiskGroupDescription group;

            uint64_t groupOffset = blockGroupStart + sizeof(DiskGroupDescription) * currentBlockGroup;

            if(!partition->ReadUnaligned(&group, offset + groupOffset, sizeof(DiskGroupDescription)))
            {
                return false;
            }

            uint64_t inodeOffset = group.inodeTable * blockSize + superBlock.inodeSize * insideBlockGroup;

            if(!partition->WriteUnaligned(&inode.inode, offset + inodeOffset, sizeof(DiskInode)))
            {
                return false;
            }

            return true;
        }

        bool Ext2FileSystem::IsValidEntry(GPT::Partition *partition)
        {
            uint8_t *temp = (uint8_t *)malloc(sizeof(uint8_t[1024]));

            memset(temp, 0, sizeof(uint8_t[1024]));

            if(!partition->Read(temp, 2, 2))
            {
                free(temp);

                return false;
            }

            DiskSuperBlock *header = (DiskSuperBlock *)temp;

            if(header->magic != EXT2FS_SIGNATURE)
            {
                free(temp);

                return false;
            }

            free(temp);

            return true;
        }

        Inode Ext2FileSystem::FindSubdir(const Inode &inode, const char *name)
        {
            Directory *directory = new Directory();

            for(uint32_t i = 0; i < inode.inode.size;)
            {
                InodeRead(directory, i, sizeof(Directory), inode);

                if(strncmp(directory->name, name, directory->directoryNameLength) == 0)
                {
                    return GetInode(directory->inode);
                }

                i += directory->directorySize;
            }

            free(directory);

            return Inode();
        }

        Inode Ext2FileSystem::GetFile(const char *path)
        {
            Inode inode = GetInode(2);

            uint32_t lastPathCursor = 0;
            uint32_t searchingFileCursor = 0;

            char file[255] = { 0 };

            for(;;)
            {
                searchingFileCursor = 0;

                for(; path[lastPathCursor] != '/' && lastPathCursor < strlen(path); lastPathCursor++, searchingFileCursor++)
                {
                    file[searchingFileCursor] = path[lastPathCursor];
                }

                lastPathCursor++;

                Inode v = FindSubdir(inode, file);

                if(!v.IsValid())
                {
                    return Inode();
                }

                if(lastPathCursor >= strlen(path))
                {
                    return v;
                }

                inode = v;
            }
        }

        uint8_t *Ext2FileSystem::ExtReadFile(const char *path)
        {
            Threading::ScopedLock Lock(lock);

            Inode inode = GetFile(path);

            if(!inode.IsValid())
            {
                return NULL;
            }

            uint8_t *data = (uint8_t *)malloc(sizeof(uint8_t[inode.inode.size]));

            if(!InodeRead(data, 0, inode.inode.size, inode))
            {
                free(data);

                return NULL;
            }

            return data;
        }

        bool Ext2FileSystem::Exists(const char *path)
        {
            Threading::ScopedLock Lock(lock);

            Inode inode = GetFile(path);

            return inode.IsValid();
        }

        uint64_t Ext2FileSystem::FileLength(const char *path)
        {
            Threading::ScopedLock Lock(lock);

            Inode inode = GetFile(path);

            if(!inode.IsValid())
            {
                return (uint64_t)-1;
            }

            uint64_t size = inode.inode.size;

            return size;
        }

        const char *Ext2FileSystem::VolumeName()
        {
            char *buffer = (char *)malloc(sizeof(superBlock.volumeName) + 1);
            buffer[sizeof(superBlock.volumeName)] = '\0';

            memcpy(buffer, superBlock.volumeName, sizeof(superBlock.volumeName));

            return buffer;
        }

        void Ext2FileSystem::Initialize(uint64_t startSector, uint64_t sectorCount)
        {
            offset = startSector * 512;

            memset(&superBlock, 0, sizeof(superBlock));

            partition->ReadUnaligned(&superBlock, offset + 1024, sizeof(DiskSuperBlock));

            blockSize = ((uint64_t)1024 << superBlock.logBlockSize);

            blockGroupDescriptorTable = blockSize == 1024 ? 2 : 1;

            rootInode = GetInode(2);
        }

        uint64_t Ext2FileSystem::ReadFile(const char *path, void *buffer, uint64_t cursor, uint64_t size)
        {
            Threading::ScopedLock Lock(lock);

            Inode inode = GetFile(path);

            if(!inode.IsValid())
            {
                return 0;
            }

            if(inode.inode.size < cursor)
            {
                return 0;
            }

            uint64_t readBytes = size;

            if(inode.inode.size < cursor + size)
            {
                readBytes = inode.inode.size - cursor;
            }

            uint8_t temp[readBytes];

            if(!InodeRead(temp, cursor, readBytes, inode))
            {
                return 0;
            }

            memcpy(buffer, temp, readBytes < size ? readBytes : size);

            return readBytes;
        }

        uint64_t Ext2FileSystem::WriteFile(const char *path, const void *buffer, uint64_t cursor, uint64_t size)
        {
            Threading::ScopedLock Lock(lock);

            Inode inode = GetFile(path);

            if(!inode.IsValid())
            {
                lock.Unlock();

                return 0;
            }

            uint64_t writeBytes = size + cursor;

            if(inode.inode.blocks * blockSize < writeBytes)
            {
                return 0;
            }
            else if(inode.inode.size < writeBytes)
            {
                inode.inode.size = writeBytes;
            }

            WriteInode(inode);

            InodeWrite(buffer, cursor, size, inode);

            return size;
        }

        uint64_t Ext2FileSystem::AllocBlockForInode(Inode &inode)
        {
            DiskGroupDescription group = ReadGroupFromGroupID(inode.ID);
            size_t gid = GroupFromInode(inode.ID);

            if(group.freeBlockCount < 1)
            {
                while(group.freeBlockCount < 1)
                {
                    group = ReadGroupFromGroupID(gid);

                    gid++;
                }
            }

            uint8_t *bitmap = (uint8_t *)malloc(blockSize);
            memset(bitmap, 0, blockSize);

            partition->ReadUnaligned(bitmap, blockSize, group.blockBitmap * blockSize + offset);

            uint64_t startBlock = 0;

            bool found = false;

            for(size_t i = 0; i < blockSize; i++)
            {
                if(!GetBit(bitmap, i))
                {
                    startBlock = i;

                    found = true;

                    break;
                }
            }

            if(!found)
            {
                free(bitmap);

                return false;
            }

            SetBit(bitmap, startBlock, 1);

            group.freeBlockCount -= 1;

            WriteGroupFromGroupID(gid, group);

            partition->WriteUnaligned(bitmap, blockSize, group.blockBitmap * blockSize + offset);

            uint64_t blockOffset = superBlock.blocksPerGroup * gid + startBlock;

            free(bitmap);

            ClearBlock(blockOffset);

            return blockOffset;
        }

        void Ext2FileSystem::ClearBlock(size_t blockAddress)
        {
            uint8_t *blockData = (uint8_t *)malloc(sizeof(uint8_t[blockSize]));

            memset(blockData, 0, blockSize);

            partition->WriteUnaligned(blockData, blockSize, blockAddress * blockSize + offset);

            free(blockData);
        }

        void Ext2FileSystem::ResizeFile(Inode &inode, uint64_t newSize)
        {
            uint64_t blockDifference = (newSize - inode.inode.size) / blockSize;
            blockDifference++;

            for(uint64_t i = 0; i < blockDifference; i++)
            {
                uint64_t result = AllocBlockForInode(inode);

                AddInodeBlockMap(inode, result);
            }

            inode.inode.blocks += blockDifference;

            WriteInode(inode);
        }

        uint64_t Ext2FileSystem::GroupFromInode(uint64_t inode)
        {
            return (inode - 1) / superBlock.inodesPerGroup;
        }

        uint64_t Ext2FileSystem::LocalGroupInodeFromInode(uint64_t inode)
        {
            return (inode - 1) % superBlock.inodesPerGroup;
        }

        void Ext2FileSystem::DebugListSubdirectory(const Inode &inode, uint32_t offset)
        {
            Directory directory;
            int32_t v = 0;

            for(uint32_t i = 0; i < inode.inode.size;)
            {
                InodeRead(&directory, i, sizeof(Directory), inode);

                for(uint32_t j = 0; j < offset; j++)
                {
                    printf("\t");
                }

                printf("%x %s (%s)\n", v, directory.name, directory.type < 8 ? Ext2DirectoryTypeNames[directory.type] : "(invalid)");

                if(directory.type == EXT2_DIRECTORY_TYPE_DIRECTORY && v >= 3)
                {
                    Inode dirInode = GetInode(directory.inode);

                    DebugListSubdirectory(dirInode, offset + 1);
                }

                i += directory.directorySize;
                v++;
            }
        }
        
        void Ext2FileSystem::DebugListDirectories()
        {
            printf("Listing directories\n");
            DebugListSubdirectory(rootInode, 0);
            printf("Listed directories\n");
        }
    }
}
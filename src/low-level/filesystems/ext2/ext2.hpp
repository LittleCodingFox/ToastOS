#pragma once
#include <stdint.h>
#include "devicemanager/GenericIODevice.hpp"
#include "filesystems/FileSystem.hpp"

namespace FileSystem
{
    namespace ext2
    {
        #define EXT2FS_SIGNATURE 0xef53

        enum Ext2FileType
        {
            EXT2_FILETYPE_REG_FILE = 1,
        	EXT2_FILETYPE_DIR = 2,
	        EXT2_FILETYPE_SYMLINK = 7
        };

        enum FileType
        {
            FILETYPE_NONE,
            FILETYPE_REGULAR,
            FILETYPE_DIRECTORY,
            FILETYPE_SYMLINK
        };

        struct __attribute__((packed)) DiskSuperBlock
        {
            uint32_t inodeCount;
            uint32_t blockCount;
            uint32_t rBlockCount;
            uint32_t freeBlockCount;
            uint32_t freeInodeCount;
            uint32_t firstDataBlock;
            uint32_t logBlockSize;
            uint32_t logFragSize;
            uint32_t blocksPerGroup;
            uint32_t fragsPerGroup;
            uint32_t inodesPerGroup;
            uint32_t mTime;
            uint32_t wTime;
            uint16_t mountCount;
            uint16_t maxMountCount;
            uint16_t magic;
            uint16_t state;
            uint16_t errors;
            uint16_t minorRevisionLevel;
            uint32_t lastCheck;
            uint32_t checkInterval;
            uint32_t creatorOS;
            uint32_t revisionLevel;
            uint16_t defResUID;
            uint16_t defResGID;
            uint32_t firstInode;
            uint16_t inodeSize;
            uint16_t blockGroupNumber;
            uint32_t featureCompatibility;
            uint32_t featureIncompatibility;
            uint32_t featureReadOnlyCompatibility;
            uint8_t uuid[16];
            uint8_t volumeName[16];
            uint8_t lastMounted[64];
            uint32_t algorithmBitmap;
            uint8_t preallocatedBlocks;
            uint8_t preallocatedDirectoryBlocks;
            uint16_t alignment;
            uint8_t journalUUID[16];
            uint32_t journalInumber;
            uint32_t journalDev;
            uint32_t lastOrphan;
            uint32_t hashSeed[4];
            uint8_t definedHashVersion;
            uint8_t padding[3];
            uint32_t defaultMountOptions;
            uint32_t firstMetaBackground;
            uint8_t reserved0[760];
        };

        static_assert(sizeof(DiskSuperBlock) == 1024, "Invalid DiskSuperBlock size");

        struct __attribute__((packed)) DiskGroupDescription
        {
            uint32_t blockBitmap;
            uint32_t inodeBitmap;
            uint32_t inodeTable;
            uint16_t freeBlockCount;
            uint16_t freeInodeCount;
            uint16_t usedDirCount;
            uint16_t pad;
            uint8_t reserved0[12];
        };

        static_assert(sizeof(DiskGroupDescription) == 32, "Invalid DiskGroupDescription size");

        struct __attribute__((packed)) DiskInode
        {
            uint16_t mode;
            uint16_t uid;
            uint32_t size;
            uint32_t atime;
            uint32_t ctime;
            uint32_t mtime;
            uint32_t dtime;
            uint16_t GID;
            uint16_t linkCount;
            uint32_t blocks;
            uint32_t flags;
            uint32_t osdl;
            uint32_t blockPointer[12];
            uint32_t singleIndirectBlockPointer;
            uint32_t doubleIndirectBlockPointer;
            uint32_t tripleIndirectBlockPointer;
            uint32_t generation;
            uint32_t fileAcl;
            uint32_t dirAcl;
            uint32_t faddr;
            uint8_t osd2[12];
        };

        static_assert(sizeof(DiskInode) == 128, "Invalid DiskInode size");

        enum
        {
            EXT2_ROOT_INO = 2,
            EXT2_S_IFMT = 0xF000,
        	EXT2_S_IFLNK = 0xA000,
	        EXT2_S_IFREG = 0x8000,
	        EXT2_S_IFDIR = 0x4000,
        };

        enum Ext2DirectoryType
        {
            EXT2_DIRECTORY_TYPE_UNKNOWN,
            EXT2_DIRECTORY_TYPE_REGULAR,
            EXT2_DIRECTORY_TYPE_DIRECTORY,
            EXT2_DIRECTORY_TYPE_CHAR_DEVICE,
            EXT2_DIRECTORY_TYPE_BLOCK_DEVICE,
            EXT2_DIRECTORY_TYPE_FIFO,
            EXT2_DIRECTORY_TYPE_SOCKET,
            EXT2_DIRECTORY_TYPE_LINK
        };

        extern const char *Ext2DirectoryTypeNames[8];

        struct __attribute__((packed)) Directory
        {
            uint32_t inode;
            uint16_t directorySize;
            uint8_t directoryNameLength;
            uint8_t type;
            char name[255];
        };

        struct __attribute__((packed)) DiskDirEntry
        {
            uint32_t inode;
            uint16_t recordLength;
            uint8_t nameLength;
            uint8_t fileType;
            char name[];
        };

        struct __attribute__((packed)) DirEntry
        {
            uint32_t inode;
            FileType fileType;
        };

        class Inode
        {
        public:
            DiskInode inode;
            size_t ID;
            size_t lastFreeGID;

            Inode() : ID(0), lastFreeGID(0)
            {
                inode = { 0 };
            }

            Inode(Inode &other);
            Inode(const Inode &other);

            inline uint32_t Size()
            {
                return inode.size;
            }

            inline bool IsValid()
            {
                return ID != 0;
            }

            inline bool IsSymlink() const
            {
                return (inode.mode & EXT2_S_IFMT) == EXT2_S_IFLNK;
            }

            inline bool IsFile() const
            {
                return (inode.mode & EXT2_S_IFMT) == EXT2_S_IFREG;
            }

            inline bool IsDirectory() const
            {
                return (inode.mode & EXT2_S_IFMT) == EXT2_S_IFDIR;
            }
        };

        class Ext2FileSystem : public FileSystem
        {
        private:
            Threading::AtomicLock extLock;
            uint64_t inodePerGroup;
            uint64_t GroupFromInode(uint64_t inode);
            uint64_t LocalGroupInodeFromInode(uint64_t inode);
            DiskSuperBlock superBlock;
            Inode rootInode;
            uint64_t blockGroupDescriptorTable;
            uint64_t blockSize;
            uint64_t offset;

            DiskGroupDescription ReadGroupFromInode(uint64_t inode);
            void WriteGroupFromInode(uint64_t inode, const DiskGroupDescription &group);
            DiskGroupDescription ReadGroupFromGroupID(uint64_t GID);
            void WriteGroupFromGroupID(uint64_t GID, const DiskGroupDescription &group);
            void ClearBlock(size_t blockAddress);
            bool InodeRead(void *buffer, uint64_t cursor, uint64_t count, const Inode &parent);
            bool InodeWrite(const void *buffer, uint64_t cursor, uint64_t count, const Inode &parent);
            void ReadBlock(uint64_t blockID, void *buffer);
            void ReadBlocks(uint64_t blockID, uint64_t length, void *buffer);
            uint64_t GetFolder(uint64_t folderID);
            uint64_t GetSimpleFile(const char *name, uint64_t forceParent = -1);
            uint32_t *CreateInodeBlockMap(const Inode &inode);
            Inode GetInode(uint64_t inode);
            bool WriteInode(const Inode &inode);
            uint8_t *ExtReadFile(const char *path);
            Inode GetFile(const char *path);
            Inode FindSubdir(const Inode &inode, const char *name);
            uint64_t GetInodeBlockMap(const Inode &inode, uint64_t blockID);
            void ResizeFile(Inode &inode, uint64_t newSize);
            uint64_t AllocBlockForInode(Inode &inode);
            void AddInodeBlockMap(Inode &inode, uint32_t blockAddress);
            void DebugListSubdirectory(const Inode &inode, uint32_t offset);
        public:
            Ext2FileSystem(GPT::Partition *partition) : FileSystem(partition)
            {
                Initialize(0, partition->GetSectorCount());
            }

            virtual void Initialize(uint64_t sector, uint64_t sectorCount) override;
            virtual bool Exists(const char *path) override;
            virtual uint64_t FileLength(const char *path) override;
            virtual uint64_t ReadFile(const char *path, void *buffer, uint64_t cursor, uint64_t size) override;
            virtual uint64_t WriteFile(const char *path, const void *buffer, uint64_t cursor, uint64_t size) override;
            virtual void DebugListDirectories() override;

            virtual const char *VolumeName() override
            {
                char *buffer = (char *)malloc(sizeof(superBlock.volumeName) + 1);
                buffer[sizeof(superBlock.volumeName)] = '\0';

                memcpy(buffer, superBlock.volumeName, sizeof(superBlock.volumeName));

                return buffer;
            }

            static bool IsValidEntry(GPT::Partition *partition);
        };
    }
}
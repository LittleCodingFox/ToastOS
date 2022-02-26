#pragma once

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "kernel.h"
#include "devicemanager/GenericIODevice.hpp"
#include "filesystems/FileSystem.hpp"

struct PACKED Ext2Superblock
{
    uint32_t inodeCount;
    uint32_t blockCount;
    uint32_t reservedBlockCount;
    uint32_t freeBlockCount;
    uint32_t freeInodeCount;
    uint32_t firstDataBlock;
    uint32_t logBlockSize;
    uint32_t logFragSize;
    uint32_t blocksPerGroup;
    uint32_t fragsPerGroup;
    uint32_t inodesPerGroup;
    uint32_t mtime;
    uint32_t wtime;

    uint16_t mountCount;
    uint16_t maxMountCount;
    uint16_t magic;
    uint16_t state;
    uint16_t errors;
    uint16_t minorRevLevel;

    uint32_t lastCheck;
    uint32_t checkInterval;
    uint32_t creatorOS;
    uint32_t revLevel;
    uint16_t defResUID;
    uint16_t defGID;

    uint32_t firstInode;

    uint16_t inodeSize;
    uint16_t blockGroupNumber;

    uint32_t featureCompat;
    uint32_t featureIncompat;
    uint32_t featureROCompat;

    uint8_t UUID[16];
    uint8_t volumeName[16];

    uint8_t lastMounted[64];

    uint32_t compressionInfo;
    uint8_t preallocBlocks;
    uint8_t preallocDirBlocks;
    uint16_t reservedGDTBlocks;
    uint8_t journalUUID[16];
    uint32_t journalInode;
    uint32_t journalDev;
    uint32_t lastOrphan;
    uint32_t hashSeed[4];
    uint8_t hashVersion;
    uint8_t padding[3];
    uint32_t defaultMountOptions;
    uint32_t firstMetaBG;
    uint8_t unused[760];
};

static_assert(sizeof(Ext2Superblock) == 1024);

struct PACKED Ext2LinuxInfo
{
    uint8_t fragNumber;
    uint8_t fragSize;

    uint16_t reserved0;

    uint16_t UIDHigh;
    uint16_t GIDHigh;

    uint32_t reserved1;
};

struct PACKED Ext2Inode
{
    uint16_t mode;
    uint16_t UID;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t GID;
    uint16_t linkCount;
    uint32_t blockCount;
    uint32_t flags;
    uint32_t osd1;
    uint32_t blocks[15];
    uint32_t generation;
    uint32_t eab;
    uint32_t major;
    uint32_t fragBlock;

    Ext2LinuxInfo osd2;
};

struct PACKED Ext2DirEntry
{
    uint32_t inode;
    uint16_t recordLength;
    uint8_t nameLength;
    uint8_t type;
};

class Ext2FileSystem : public FileSystem
{
private:
    struct FileHandleData
    {
        uint64_t ID;
        uint64_t length;

        Ext2Inode inode;

        int currentEntry = 0;
        vector<uint32_t> allocMap;
        dirent dir;
    };

    uint64_t fileHandleCounter;

    vector<FileHandleData> fileHandles;

    Ext2Superblock superblock;
    Ext2Inode root;
    uint64_t blockSize;

    bool initOK;

    FileHandleData *GetHandle(FileSystemHandle handle);
    void ListSubdirs(Ext2Inode *inode, const vector<uint32_t> &allocMap, uint32_t indentation);
    bool FindInode(Ext2Inode *result, uint64_t inode);
    vector<uint32_t> CreateAllocMap(Ext2Inode *inode);
    bool ParseDirent(Ext2DirEntry *dirent, const char *path);
    bool SymlinkToInode(Ext2Inode *inode);
    bool ReadInode(void *buffer, uint64_t position, uint64_t count, const vector<uint32_t> &allocMap);
    bool ReadDirent(Ext2DirEntry *dirent, const vector<uint32_t> &allocMap, uint64_t offset);
    bool ReadDirentName(Ext2DirEntry *dirent, const vector<uint32_t> &allocMap, char *buffer, uint64_t offset);

public:

    static bool IsValid(GPT::Partition *partition);

    Ext2FileSystem(GPT::Partition *partition) : FileSystem(partition), fileHandleCounter(0), initOK(false)
    {
        Initialize(0, partition->GetSectorCount());
    }

    virtual void Initialize(uint64_t sector, uint64_t sectorCount) override;

    virtual bool Exists(const char *path) override;

    virtual FileSystemHandle GetFileHandle(const char *path) override;
    virtual void DisposeFileHandle(FileSystemHandle handle) override;
    virtual int FileHandleType(FileSystemHandle handle) override;

    virtual uint64_t FileLength(FileSystemHandle handle) override;
    virtual string FileLink(FileSystemHandle handle) override;

    virtual uint64_t ReadFile(FileSystemHandle handle, void *buffer, uint64_t cursor, uint64_t size) override;
    virtual uint64_t WriteFile(FileSystemHandle handle, const void *buffer, uint64_t cursor, uint64_t size) override;

    virtual void DebugListDirectories() override;

    virtual const char *VolumeName() override;

    virtual struct stat Stat(FileSystemHandle handle) override;

    virtual dirent *ReadEntries(FileSystemHandle handle) override;

    virtual void CloseDir(FileSystemHandle handle) override;
};

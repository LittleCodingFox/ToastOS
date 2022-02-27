#pragma once

#include <frg/manual_box.hpp>
#include "gpt/GPT.hpp"
#include "filesystems/FileSystem.hpp"

class PartitionManager
{
private:
    struct DiskInfo
    {
        GenericIODevice *device;
        GPT::PartitionTable *table;
        FileSystem *fileSystem;

        DiskInfo() : device(NULL), table(NULL), fileSystem(NULL) {}
    };

    frg::vector<DiskInfo, frg_allocator> disks;    
public:
    void Initialize();
};

extern box<PartitionManager> globalPartitionManager;

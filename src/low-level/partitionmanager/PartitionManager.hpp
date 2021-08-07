#pragma once

#include "gpt/GPT.hpp"
#include "filesystems/FileSystem.hpp"

namespace FileSystem
{
    class PartitionManager
    {
    private:
        struct DiskInfo
        {
            Devices::GenericIODevice *device;
            GPT::PartitionTable *table;
            FileSystem *fileSystem;

            DiskInfo() : device(NULL), table(NULL), fileSystem(NULL) {}
        };

        DynamicArray<DiskInfo> disks;    
    public:
        void Initialize();
    };

    extern PartitionManager globalPartitionManager;
}

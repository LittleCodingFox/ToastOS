#pragma once

#include "gpt/GPT.hpp"

namespace FileSystem
{
    class PartitionManager
    {
    private:
        struct DiskInfo
        {
            Devices::GenericIODevice *device;
            GPT::PartitionTable *table;
        };

        DynamicArray<DiskInfo> disks;    
    public:
        void Initialize();
    };

    extern PartitionManager globalPartitionManager;
}

#pragma once

#include <frg/manual_box.hpp>
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

        frg::vector<DiskInfo, frg_allocator> disks;    
    public:
        void Initialize();
    };

    extern frg::manual_box<PartitionManager> globalPartitionManager;
}

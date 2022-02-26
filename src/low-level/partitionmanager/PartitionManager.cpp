#include "PartitionManager.hpp"
#include "filesystems/VFS.hpp"
#include "elf/elf.hpp"
#include "debug.hpp"
#include <stdio.h>
#include "filesystems/ext2/ext2.hpp"

frg::manual_box<PartitionManager> globalPartitionManager;

void PartitionManager::Initialize()
{
    frg::vector<GenericDevice *, frg_allocator> diskDevices = globalDeviceManager.GetDevices(DEVICE_TYPE_DISK);

    printf("[PartitionManager] Found %i disk devices\n", diskDevices.size());

    for(uint32_t i = 0; i < diskDevices.size(); i++)
    {
        if(diskDevices[i] == NULL)
        {
            continue;
        }

        DiskInfo diskInfo;
        diskInfo.device = (GenericIODevice *)diskDevices[i];

        printf("[PartitionManager] Getting partition table for %u\n", i);

        diskInfo.table = new GPT::PartitionTable(diskInfo.device);

        if(!diskInfo.table->Parse())
        {
            printf("[PartitionManager] Failed to parse partitions for %u\n", i);

            delete diskInfo.table;

            continue;
        }

        printf("[PartitionManager] Added %u partitions for %u\n", diskInfo.table->PartitionCount(), i);

        disks.push_back(diskInfo);
    }

    for(uint32_t i = 0; i < disks.size(); i++)
    {
        DiskInfo &disk = disks[i];

        printf("[PartitionManager] Partitions for disk %s:\n", disk.device->name());

        for(uint32_t j = 0; j < disk.table->PartitionCount(); j++)
        {
            GPT::Partition &partition = disk.table->GetPartition(j);

            printf("[PartitionManager] \t%s with size: %s, type: %s)\n", partition.GetID().ToString(), partition.SizeString(), partition.GetType().ToString());

            if(Ext2FileSystem::IsValid(&partition))
            {
                disk.fileSystem = new Ext2FileSystem(&partition);
            }

            if(disk.fileSystem != NULL)
            {
                printf("[PartitionManager] \tFound volume %s\n", disk.fileSystem->VolumeName());

                vfs->AddMountPoint("/", disk.fileSystem);

                //disk.fileSystem->DebugListDirectories();
            }
        }
    }
}

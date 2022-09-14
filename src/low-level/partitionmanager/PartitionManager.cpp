#include "PartitionManager.hpp"
#include "filesystems/VFS.hpp"
#include "elf/elf.hpp"
#include "debug.hpp"
#include <stdio.h>

frg::manual_box<PartitionManager> globalPartitionManager;

void PartitionManager::Initialize()
{
    frg::vector<GenericDevice *, frg_allocator> diskDevices = globalDeviceManager.GetDevices(DEVICE_TYPE_DISK);

    DEBUG_OUT("[PartitionManager] Found %i disk devices", diskDevices.size());

    for(uint32_t i = 0; i < diskDevices.size(); i++)
    {
        if(diskDevices[i] == NULL)
        {
            continue;
        }

        DiskInfo diskInfo;
        diskInfo.device = (GenericIODevice *)diskDevices[i];

        DEBUG_OUT("[PartitionManager] Getting partition table for %u", i);

        diskInfo.table = new GPT::PartitionTable(diskInfo.device);

        if(!diskInfo.table->Parse())
        {
            DEBUG_OUT("[PartitionManager] Failed to parse partitions for %u", i);

            delete diskInfo.table;

            continue;
        }

        DEBUG_OUT("[PartitionManager] Added %u partitions for %u", diskInfo.table->PartitionCount(), i);

        disks.push_back(diskInfo);
    }

    for(uint32_t i = 0; i < disks.size(); i++)
    {
        DiskInfo &disk = disks[i];

        DEBUG_OUT("[PartitionManager] Partitions for disk %s:", disk.device->name());

        for(uint32_t j = 0; j < disk.table->PartitionCount(); j++)
        {
            GPT::Partition &partition = disk.table->GetPartition(j);

            DEBUG_OUT("[PartitionManager] \t%s with size: %s, type: %s)", partition.GetID().ToString(), partition.SizeString(), partition.GetType().ToString());

/*
            if(disk.fileSystem != NULL)
            {
                DEBUG_OUT("Found volume %s", disk.fileSystem->VolumeName());

                //TODO: Dynamic mount points
                vfs->AddMountPoint("/", disk.fileSystem);

                //disk.fileSystem->DebugListDirectories();
            }
*/
        }
    }
}

#include "GPT.hpp"
#include "debug.hpp"

namespace GPT
{
    PartitionTable::PartitionTable(GenericIODevice *device) : device(device)
    {
    }

    bool PartitionTable::Parse()
    {
        uint8_t *headerBuffer = new uint8_t[512];

        if(!device->Read(headerBuffer, 1, 1))
        {
            delete [] headerBuffer;

            return false;
        }

        DiskHeader *header = (DiskHeader *)headerBuffer;

        size_t tableSize = header->entrySize * header->entryCount;
        size_t tableSectors = tableSize / 512;

        if(tableSize % 512 == 0)
        {
            tableSectors++;
        }

        uint8_t *tableBuffer = new uint8_t[tableSectors * 512];

        if(!device->Read(tableBuffer, 2, tableSectors))
        {
            delete [] tableBuffer;
            delete [] headerBuffer;

            return false;
        }

        for(uint32_t i = 0; i < header->entryCount; i++)
        {
            DiskEntry *entry = (DiskEntry *)((uint8_t *)tableBuffer + i * header->entrySize);

            if(entry->typeGuid.equals(NullGuid))
            {
                continue;
            }

            partitions.push_back(new Partition(*this, entry->uniqueGuid, entry->typeGuid, entry->firstLBA, entry->lastLBA - entry->firstLBA + 1));
        }

        delete [] tableBuffer;
        delete [] headerBuffer;

        return true;
    }

    GenericIODevice *PartitionTable::GetDevice() const
    {
        return device;
    }

    uint32_t PartitionTable::PartitionCount() const
    {
        return partitions.size();
    }

    Partition &PartitionTable::GetPartition(int index)
    {
        return *partitions[index];
    }

    Partition::Partition(PartitionTable &table, Guid ID, Guid type, uint64_t startLBA, uint64_t sectorCount) : table(table),
        ID(ID), type(type), startLBA(startLBA), sectorCount(sectorCount)
    {
    }

    Guid Partition::GetType() const
    {
        return type;
    }

    Guid Partition::GetID() const
    {
        return ID;
    }

    uint64_t Partition::GetSectorCount() const
    {
        return sectorCount;
    }

    string Partition::SizeString() const
    {
        uint64_t size = sectorCount * 512;

        char sizes[6] = {
            ' ',
            'K',
            'M',
            'G',
            'T',
            'H'
        };

        uint8_t sizeIndex = 0;

        while(size >= 1024)
        {
            sizeIndex++;

            size /= 1024;
        }

        char *buffer = (char *)malloc(512);

        sprintf(buffer, "%lu %cB", size, sizes[sizeIndex]);

        string outString = buffer;

        delete [] buffer;

        return outString;
    }

    bool Partition::Read(void *data, uint64_t sector, uint64_t count)
    {
        if(sector + count > sectorCount)
        {
            return false;
        }

        return table.GetDevice()->Read(data, startLBA + sector, count);
    }

    bool Partition::Write(const void *data, uint64_t sector, uint64_t count)
    {
        if(sector + count > sectorCount)
        {
            return false;
        }

        return table.GetDevice()->Write(data, startLBA + sector, count);
    }
}

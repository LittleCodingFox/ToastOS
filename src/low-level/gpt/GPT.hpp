#pragma once
#include <stdint.h>
#include <string.h>
#include <frg_allocator.hpp>
#include "printf/printf.h"
#include "frg/vector.hpp"
#include "low-level/devicemanager/GenericIODevice.hpp"

namespace GPT
{
    struct __attribute__((packed)) Guid
    {
        uint32_t a;
        uint16_t b;
        uint16_t c;
        uint8_t d[2];
        uint8_t e[6];

        inline bool equals(const Guid &other) const
        {
            return a == other.a &&
                b == other.b &&
                c == other.c &&
                memcmp(d, other.d, 2) == 0 &&
                memcmp(e, other.e, 6) == 0;
        }

        const char *ToString() const
        {
            char *buffer = (char *)malloc((sizeof(uint32_t) + sizeof(uint16_t[2]) + sizeof(uint8_t[8])) * 2 + 5);

            sprintf(buffer, "%x-%x-%x-%x-%x%x%x%x%x%x%x%x", a, b, c, d[0], d[1], e[0], e[1], e[2], e[3], e[4], e[5]);

            return buffer;
        }
    };

    static_assert(sizeof(Guid) == 16, "Guid has invalid size");

    constexpr Guid NullGuid { 0, 0, 0, { 0, 0 }, { 0, 0, 0, 0, 0, 0 } };
    constexpr Guid WindowsDataGuid { 0xEBD0A0A2, 0xB9E5, 0x4433, {0x87, 0xC0},
			{0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7} };

    struct __attribute__((packed)) DiskHeader
    {
        uint64_t signature;
        uint32_t revision;
        uint32_t headerSize;
        uint32_t headerChecksum;
        uint32_t reserved0;
        uint64_t currentLBA;
        uint64_t backupLBA;
        uint64_t firstLBA;
        uint64_t lastLBA;
        uint8_t guid[16];
        uint64_t startingLBA;
        uint32_t entryCount;
        uint32_t entrySize;
        uint32_t tableChecksum;
        uint8_t padding[420];
    };

    static_assert(sizeof(DiskHeader) == 512, "DiskHeader has invalid size");

    struct __attribute__((packed)) DiskEntry
    {
        Guid typeGuid;
        Guid uniqueGuid;
        uint64_t firstLBA;
        uint64_t lastLBA;
        uint64_t attributeFlags;
        uint8_t partitionName[72];
    };

    static_assert(sizeof(DiskEntry) == 128, "DiskEntry has invalid size");

    class Partition;

    struct PartitionTable
    {
    public:
        PartitionTable(Devices::GenericIODevice *device);

        Devices::GenericIODevice *GetDevice() const;

        bool Parse();

        uint32_t PartitionCount() const;

        Partition &GetPartition(int index);

    private:
        Devices::GenericIODevice *device;
        frg::vector<Partition *, frg_allocator> partitions;
    };

    class Partition : public Devices::GenericIODevice
    {
    public:
        Partition(PartitionTable &table, Guid ID, Guid type, uint64_t startLBA, uint64_t sectorCount);

        Guid GetID() const;
        Guid GetType() const;
        uint64_t GetSectorCount() const;
        const char *SizeString() const;

        virtual bool Read(void *data, uint64_t cursor, uint64_t count) override;
        virtual bool Write(const void *data, uint64_t cursor, uint64_t count) override;

    private:
        PartitionTable &table;
        Guid ID;
        Guid type;
        uint64_t startLBA;
        uint64_t sectorCount;
    };
}
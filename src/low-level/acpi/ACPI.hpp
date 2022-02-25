#pragma once
#include "kernel.h"

struct PACKED RSDP2
{
    uint8_t signature[8];
    uint8_t checksum;
    uint8_t OEMID[6];
    uint8_t revision;
    uint32_t RSDTAddress;
    uint32_t length;
    uint64_t XSDTAddress;
    uint8_t extendedChecksum;
    uint8_t reserved[3];
};

struct PACKED SDTHeader
{
    uint8_t signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    uint8_t OEMID[6];
    uint8_t OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t CreatorID;
    uint32_t CreatorRevision;
};

struct PACKED MCFGHeader
{
    SDTHeader header;
    uint64_t reserved;
};

struct PACKED ACPIDeviceConfig
{
    uint64_t baseAddress;
    uint16_t segmentationGroup;
    uint8_t startBus;
    uint8_t endBus;
    uint32_t reserved;
};

namespace ACPI
{
    void DumpTables(volatile SDTHeader *header);
    void *FindTable(volatile SDTHeader *header, const char *signature);
}

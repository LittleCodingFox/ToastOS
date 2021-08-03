#pragma once
#include "acpi/ACPI.hpp"

#include <stdint.h>

struct PCIDeviceHeader
{
    uint16_t vendorID;
    uint16_t deviceID;
    uint16_t command;
    uint16_t status;
    uint8_t revisionID;
    uint8_t progIF;
    uint8_t objectSubclass;
    uint8_t objectClass;
    uint8_t cacheLineSize;
    uint8_t latencyTimer;
    uint8_t headerType;
    uint8_t BIST;
};

struct PCIHeader0
{
    PCIDeviceHeader *header;
    uint32_t BAR0;
    uint32_t BAR1;
    uint32_t BAR2;
    uint32_t BAR3;
    uint32_t BAR4;
    uint32_t BAR5;
    uint32_t cardbusCISPointer;
    uint16_t subsystemVendorID;
    uint16_t subsystemID;
    uint32_t expansionROMBaseAddr;
    uint8_t capabilitiesPtr;
    uint8_t reserved0;
    uint16_t reserved1;
    uint32_t reserved2;
    uint8_t interruptLine;
    uint8_t interruptPin;
    uint8_t minGrant;
    uint8_t maxLatency;
};

namespace PCI
{
    void enumeratePCI(MCFGHeader *mcfg);

    extern const char *deviceClasses[];

    const char *vendorName(uint16_t vendorID);
    const char *deviceName(uint16_t vendorID, uint16_t deviceID);
    const char *subclassName(uint8_t classCode, uint8_t subclassCode);
    const char *progIFName(uint8_t classCode, uint8_t subclassCode, uint8_t progIF);
}
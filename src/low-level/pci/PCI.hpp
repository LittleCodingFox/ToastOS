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

namespace PCI
{
    void enumeratePCI(MCFGHeader *mcfg);

    extern const char *deviceClasses[];

    const char *vendorName(uint16_t vendorID);
    const char *deviceName(uint16_t vendorID, uint16_t deviceID);
    const char *subclassName(uint8_t classCode, uint8_t subclassCode);
    const char *progIFName(uint8_t classCode, uint8_t subclassCode, uint8_t progIF);
}
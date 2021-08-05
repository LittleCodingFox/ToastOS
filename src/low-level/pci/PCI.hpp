#pragma once
#include "acpi/ACPI.hpp"

#include <stdint.h>

namespace PCI
{
    struct Bar
    {
        uint64_t address;
        uint8_t type;
        uint8_t layoutType;
        bool prefetchable;
    };

    struct Device
    {
        uint16_t vendorID;
        uint16_t deviceID;
        uint8_t classCode;
        Bar bars[6];
    };

    void EnumeratePCI(volatile MCFGHeader *mcfg);

    extern const char *DeviceClasses[];

    const char *VendorName(uint16_t vendorID);
    const char *DeviceName(uint16_t vendorID, uint16_t deviceID);
    const char *SubclassName(uint8_t classCode, uint8_t subclassCode);
    const char *ProgIFName(uint8_t classCode, uint8_t subclassCode, uint8_t progIF);
}
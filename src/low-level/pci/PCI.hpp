#pragma once
#include "acpi/ACPI.hpp"

#include <stdint.h>

namespace PCI
{
    struct Bar
    {
        uint64_t address;
        uint8_t type;
        bool prefetchable;
    };

    struct Device
    {
        uint16_t vendorID;
        uint16_t deviceID;
        uint8_t classCode;
        Bar bars[6];
    };

    void enumeratePCI(volatile MCFGHeader *mcfg);

    extern const char *deviceClasses[];

    const char *vendorName(uint16_t vendorID);
    const char *deviceName(uint16_t vendorID, uint16_t deviceID);
    const char *subclassName(uint8_t classCode, uint8_t subclassCode);
    const char *progIFName(uint8_t classCode, uint8_t subclassCode, uint8_t progIF);
}
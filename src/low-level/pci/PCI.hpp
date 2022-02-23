#pragma once
#include "acpi/ACPI.hpp"

#include <stdint.h>
#include "kernel.h"

struct PCIBar
{
    uint64_t address;
    uint8_t type;
    uint8_t layoutType;
    bool prefetchable;
};

struct PCIDevice
{
    uint16_t deviceID;
    uint16_t vendorID;

    uint8_t bus;
    uint8_t slot;
    uint8_t function;

    uint8_t classCode;
    uint8_t subclass;
    uint8_t progIf;

    PCIBar bars[6];
};

void SetPCIMCFG(volatile MCFGHeader *mcfg);

void EnumeratePCI();
void EnumeratePCIDevices(uint16_t deviceID, uint16_t vendorID, void (*callback)(const PCIDevice &device));
void EnumerateGenericPCIDevices(uint8_t classCode, uint8_t subclass, void (*callback)(const PCIDevice &device));

extern const char *PCIDeviceClasses[];

string PCIVendorName(uint16_t vendorID);
string PCIDeviceName(uint16_t vendorID, uint16_t deviceID);
string PCISubclassName(uint8_t classCode, uint8_t subclassCode);
string PCIProgIFName(uint8_t classCode, uint8_t subclassCode, uint8_t progIF);

#include "PCI.hpp"
#include "paging/PageTableManager.hpp"
#include "../drivers/AHCI/AHCIDriver.hpp"
#include "devicemanager/DeviceManager.hpp"
#include "printf/printf.h"
#include "debug.hpp"
#include <frg/formatting.hpp>

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

struct PCIDeviceHolder
{
    vector<PCIDevice> devices;
};

box<PCIDeviceHolder> PCIDevices;

volatile MCFGHeader *mcfg = NULL;

void SetPCIMCFG(volatile MCFGHeader *header)
{
    mcfg = header;
}

const char* PCIDeviceClasses[]
{
    "Unclassified",
    "Mass Storage Controller",
    "Network Controller",
    "Display Controller",
    "Multimedia Controller",
    "Memory Controller",
    "Bridge Device",
    "Simple Communication Controller",
    "Base System Peripheral",
    "Input Device Controller",
    "Docking Station", 
    "Processor",
    "Serial Bus Controller",
    "Wireless Controller",
    "Intelligent Controller",
    "Satellite Communication Controller",
    "Encryption Controller",
    "Signal Processing Controller",
    "Processing Accelerator",
    "Non Essential Instrumentation"
};

string PCIVendorName(uint16_t vendorID)
{
    switch (vendorID)
    {
        case 0x8086:
            return "Intel Corp";

        case 0x1022:
            return "AMD";

        case 0x10DE:
            return "NVIDIA Corporation";
    }

    char buffer[20];

    sprintf(buffer, "%x", vendorID);

    return buffer;
}

string PCIDeviceName(uint16_t vendorID, uint16_t deviceID)
{
    switch (vendorID)
    {
        case 0x8086: // Intel
            switch(deviceID)
            {
                case 0x29C0:
                    return "Express DRAM Controller";

                case 0x2918:
                    return "LPC Interface Controller";

                case 0x2922:
                    return "6 port SATA Controller [AHCI mode]";

                case 0x2930:
                    return "SMBus Controller";
            }

            break;
    }

    char buffer[20];

    sprintf(buffer, "%x", deviceID);

    return buffer;
}

string MassStorageControllerSubclassName(uint8_t subclassCode)
{
    switch (subclassCode)
    {
        case 0x00:
            return "SCSI Bus Controller";

        case 0x01:
            return "IDE Controller";

        case 0x02:
            return "Floppy Disk Controller";

        case 0x03:
            return "IPI Bus Controller";

        case 0x04:
            return "RAID Controller";

        case 0x05:
            return "ATA Controller";

        case 0x06:
            return "Serial ATA";

        case 0x07:
            return "Serial Attached SCSI";

        case 0x08:
            return "Non-Volatile Memory Controller";

        case 0x80:
            return "Other";
    }

    char buffer[20];

    sprintf(buffer, "%x", subclassCode);

    return buffer;
}

string SerialBusControllerSubclassName(uint8_t subclassCode)
{
    switch (subclassCode)
    {
        case 0x00:
            return "FireWire (IEEE 1394) Controller";

        case 0x01:
            return "ACCESS Bus";

        case 0x02:
            return "SSA";

        case 0x03:
            return "USB Controller";

        case 0x04:
            return "Fibre Channel";

        case 0x05:
            return "SMBus";

        case 0x06:
            return "Infiniband";

        case 0x07:
            return "IPMI Interface";

        case 0x08:
            return "SERCOS Interface (IEC 61491)";

        case 0x09:
            return "CANbus";

        case 0x80:
            return "SerialBusController - Other";
    }

    char buffer[20];

    sprintf(buffer, "%x", subclassCode);

    return buffer;
}

string BridgeDeviceSubclassName(uint8_t subclassCode)
{
    switch (subclassCode)
    {
        case 0x00:
            return "Host Bridge";

        case 0x01:
            return "ISA Bridge";

        case 0x02:
            return "EISA Bridge";

        case 0x03:
            return "MCA Bridge";

        case 0x04:
            return "PCI-to-PCI Bridge";

        case 0x05:
            return "PCMCIA Bridge";

        case 0x06:
            return "NuBus Bridge";

        case 0x07:
            return "CardBus Bridge";

        case 0x08:
            return "RACEway Bridge";

        case 0x09:
            return "PCI-to-PCI Bridge";

        case 0x0a:
            return "InfiniBand-to-PCI Host Bridge";

        case 0x80:
            return "Other";
    }

    char buffer[20];

    sprintf(buffer, "%x", subclassCode);

    return buffer;
}

string PCISubclassName(uint8_t classCode, uint8_t subclassCode)
{
    switch (classCode)
    {
        case 0x01:
            return MassStorageControllerSubclassName(subclassCode);

        case 0x03:
            switch (subclassCode)
            {
                case 0x00:
                    return "VGA Compatible Controller";
            }

        case 0x06:
            return BridgeDeviceSubclassName(subclassCode);

        case 0x0C:
            return SerialBusControllerSubclassName(subclassCode);
    }

    char buffer[20];

    sprintf(buffer, "%x", subclassCode);

    return buffer;
}

string PCIProgIFName(uint8_t classCode, uint8_t subclassCode, uint8_t progIF)
{
    switch (classCode)
    {
        case 0x01:
            switch (subclassCode)
            {
                case 0x06:
                    switch (progIF)
                    {
                        case 0x00:
                            return "Vendor Specific Interface";
                        case 0x01:
                            return "AHCI 1.0";
                        case 0x02:
                            return "Serial Storage Bus";
                    }
            }

        case 0x03:
            switch (subclassCode)
            {
                case 0x00:
                    switch (progIF)
                    {
                        case 0x00:
                            return "VGA Controller";
                        case 0x01:
                            return "8514-Compatible Controller";
                    }

                    break;
            }

        case 0x0C:
            switch (subclassCode)
            {
                case 0x03:
                    switch (progIF)
                    {
                        case 0x00:
                            return "UHCI Controller";

                        case 0x10:
                            return "OHCI Controller";

                        case 0x20:
                            return "EHCI (USB2) Controller";

                        case 0x30:
                            return "XHCI (USB3) Controller";

                        case 0x80:
                            return "Unspecified";

                        case 0xFE:
                            return "USB Device (Not a Host Controller)";
                    }

                    break;
            }    
    }

    char buffer[20];

    sprintf(buffer, "%x", progIF);

    return buffer;
}

PCIBar DecodeBar(volatile uint8_t **ptr)
{
    uint32_t bar = *(uint32_t *)*ptr;

    *ptr += sizeof(uint32_t);

    uint8_t layoutType = bar & 1;
    uint64_t address;
    uint8_t type = 0;
    bool prefetchable = false;

    if(layoutType == 0)
    {
        prefetchable = bar & 0b1000;
        type = (bar & 0b110) >> 1;

        switch(type)
        {
            case 0x0:

                address = bar & 0xFFFFFFF0;

                break;

            case 0x02:

                uint32_t nextBar = *(uint32_t *)*ptr;

                *ptr += sizeof(uint32_t);

                address = ((uint64_t)bar & 0xFFFFFFF0) + (((uint64_t)nextBar & 0xFFFFFFFF) << 32);

                break;
        }
    }
    else
    {
        address = bar & 0xFFFFFFFC;
    }

    auto outValue = PCIBar();

    outValue.layoutType = layoutType;
    outValue.type = type;
    outValue.prefetchable = prefetchable;

    if(address != 0)
    {
        outValue.address = TranslateToHighHalfMemoryAddress(address);

        globalPageTableManager->MapMemory((void *)outValue.address, (void *)address,
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
    }
    else
    {
        outValue.address = 0;
    }

    return outValue;
}

void EnumerateFunction(uint64_t bus, uint64_t deviceID, uint64_t deviceAddress, uint64_t function)
{
    uint64_t offset = function << 12;

    uint64_t functionAddress = deviceAddress + offset;

    volatile PCIDeviceHeader *deviceHeader = (volatile PCIDeviceHeader *)TranslateToHighHalfMemoryAddress(functionAddress);

    if(deviceHeader->deviceID == 0 || deviceHeader->deviceID == 0xFFFF)
    {
        return;
    }

    PCIDevice device;
    device.deviceID = deviceHeader->deviceID;
    device.vendorID = deviceHeader->vendorID;

    device.bus = bus;
    device.slot = deviceID;
    device.function = function;

    device.classCode = deviceHeader->objectClass;
    device.subclass = deviceHeader->objectSubclass;
    device.progIf = deviceHeader->progIF;

    volatile uint8_t *barPtr = (volatile uint8_t *)deviceHeader;
    barPtr += 0x10; //BAR0 location

    globalPageTableManager->MapMemory((void *)barPtr, (void *)(functionAddress + 0x10), PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);

    for(uint32_t i = 0; i < 6; i++)
    {
        device.bars[i] = DecodeBar(&barPtr);
    }

    auto deviceName = PCIDeviceName(device.vendorID, device.deviceID);
    auto vendor = PCIVendorName(device.vendorID);
    auto subclass = PCISubclassName(device.classCode, device.subclass);
    auto progIf = PCIProgIFName(device.classCode, device.subclass, device.progIf);

    printf("[PCI] Added device: %s (%s) class: %s subclass: %s progIf: %s\n",
        deviceName.data(),
        vendor.data(),
        PCIDeviceClasses[device.classCode],
        subclass.data(),
        progIf.data());

    PCIDevices->devices.push_back(device);
}

void EnumerateDevice(uint64_t bus, uint64_t busAddress, uint64_t device)
{
    uint64_t offset = device << 15;

    uint64_t deviceAddress = busAddress + offset;

    volatile PCIDeviceHeader *deviceHeader = (volatile PCIDeviceHeader *)TranslateToHighHalfMemoryAddress(deviceAddress);

    if(deviceHeader->deviceID == 0 || deviceHeader->deviceID == 0xFFFF)
    {
        return;
    }

    for(uint64_t i = 0; i < 8; i++)
    {
        EnumerateFunction(bus, device, deviceAddress, i);
    }
}

void EnumerateBus(uint64_t baseAddress, uint64_t bus)
{
    uint64_t offset = bus << 20;

    uint64_t busAddress = baseAddress + offset;

    volatile PCIDeviceHeader *deviceHeader = (volatile PCIDeviceHeader *)TranslateToHighHalfMemoryAddress(busAddress);

    if(deviceHeader->deviceID == 0 || deviceHeader->deviceID == 0xFFFF)
    {
        return;
    }

    for(uint64_t i = 0; i < 32; i++)
    {
        EnumerateDevice(bus, busAddress, i);
    }
}

void EnumeratePCIDevices(uint16_t deviceID, uint16_t vendorID, void (*callback)(const PCIDevice &device))
{
    for(auto &device : PCIDevices->devices)
    {
        if(device.deviceID == deviceID && device.vendorID == vendorID)
        {
            callback(device);
        }
    }
}

void EnumerateGenericPCIDevices(uint8_t classCode, uint8_t subclass, void (*callback)(const PCIDevice &device))
{
    for(auto &device : PCIDevices->devices)
    {
        if(device.classCode == classCode && device.subclass == subclass)
        {
            callback(device);
        }
    }
}

void EnumeratePCI()
{
    if(mcfg == NULL)
    {
        return;
    }

    PCIDevices.initialize();

    uint32_t entries = (mcfg->header.length - sizeof(MCFGHeader)) / sizeof(ACPIDeviceConfig);

    for(uint32_t i = 0; i < entries; i++)
    {
        volatile ACPIDeviceConfig *deviceConfig = (volatile ACPIDeviceConfig *)((uint64_t)mcfg + sizeof(MCFGHeader) + sizeof(ACPIDeviceConfig[i]));

        for(uint64_t bus = deviceConfig->startBus; bus < deviceConfig->endBus; bus++)
        {
            EnumerateBus(deviceConfig->baseAddress, bus);
        }
    }

    printf("[PCI] Added %llu PCI devices\n", PCIDevices->devices.size());
}

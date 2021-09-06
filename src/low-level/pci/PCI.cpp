#include "PCI.hpp"
#include "cstring/cstring.hpp"
#include "paging/PageTableManager.hpp"
#include "../drivers/AHCI/AHCIDriver.hpp"
#include "devicemanager/DeviceManager.hpp"
#include "printf/printf.h"
#include "debug.hpp"

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
    const char* DeviceClasses[]
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

    const char* VendorName(uint16_t vendorID)
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
  
        return to_hstring(vendorID);
    }

    const char* DeviceName(uint16_t vendorID, uint16_t deviceID)
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

        return to_hstring(deviceID);
    }

    const char* MassStorageControllerSubclassName(uint8_t subclassCode)
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

        return to_hstring(subclassCode);
    }

    const char* SerialBusControllerSubclassName(uint8_t subclassCode)
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

        return to_hstring(subclassCode);
    }

    const char* BridgeDeviceSubclassName(uint8_t subclassCode)
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

        return to_hstring(subclassCode);
    }

    const char* SubclassName(uint8_t classCode, uint8_t subclassCode)
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

        return to_hstring(subclassCode);
    }

    const char* ProgIFName(uint8_t classCode, uint8_t subclassCode, uint8_t progIF)
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

        return to_hstring(progIF);
    }

    Bar DecodeBar(volatile uint8_t **ptr)
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

        Bar outValue = Bar();

        outValue.layoutType = layoutType;
        outValue.type = type;
        outValue.prefetchable = prefetchable;

        if(address != NULL)
        {
            outValue.address = TranslateToHighHalfMemoryAddress(address);

            globalPageTableManager->MapMemory((void *)outValue.address, (void *)address,
                PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
        }
        else
        {
            outValue.address = NULL;
        }

        return outValue;
    }

    void EnumerateFunction(uint64_t deviceAddress, uint64_t function)
    {
        uint64_t offset = function << 12;

        uint64_t functionAddress = deviceAddress + offset;

        volatile PCIDeviceHeader *deviceHeader = (volatile PCIDeviceHeader *)TranslateToHighHalfMemoryAddress(functionAddress);

        if(deviceHeader->deviceID == 0 || deviceHeader->deviceID == 0xFFFF)
        {
            return;
        }

        printf("[PCI DEVICE]\n\tVendor: %s\n\tDevice Name: %s\n\tDevice Class: %s\n\tSubclass Name: %s\n\tProgIf Name: %s\n",
                VendorName(deviceHeader->vendorID),
                DeviceName(deviceHeader->vendorID, deviceHeader->deviceID),
                DeviceClasses[deviceHeader->objectClass],
                SubclassName(deviceHeader->objectClass, deviceHeader->objectSubclass),
                ProgIFName(deviceHeader->objectClass, deviceHeader->objectSubclass, deviceHeader->progIF));

        Device *device = new Device();
        device->classCode = deviceHeader->objectClass;
        device->deviceID = deviceHeader->deviceID;
        device->vendorID = deviceHeader->vendorID;

        volatile uint8_t *barPtr = (volatile uint8_t *)deviceHeader;
        barPtr += 0x10; //BAR0 location

        globalPageTableManager->MapMemory((void *)barPtr, (void *)(functionAddress + 0x10), PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);

        for(uint32_t i = 0; i < 6; i++)
        {
            device->bars[i] = DecodeBar(&barPtr);
        }

        switch(deviceHeader->objectClass)
        {
            case 0x01: //Mass Storage Controller
                switch(deviceHeader->objectSubclass)
                {
                    case 0x06: //SATA
                        switch(deviceHeader->progIF)
                        {
                            case 0x01:
                                Drivers::AHCI::HandleMassStorageDevice(device);

                            break;
                        }

                        break;
                }

                break;
        }
    }

    void EnumerateDevice(uint64_t busAddress, uint64_t device)
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
            EnumerateFunction(deviceAddress, i);
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
            EnumerateDevice(busAddress, i);
        }
    }

    void EnumeratePCI(volatile MCFGHeader *mcfg)
    {
        printf("%s", "[PCI] Enumerating Devices\n");

        uint32_t entries = (mcfg->header.length - sizeof(MCFGHeader)) / sizeof(ACPIDeviceConfig);

        for(uint32_t i = 0; i < entries; i++)
        {
            volatile ACPIDeviceConfig *deviceConfig = (volatile ACPIDeviceConfig *)((uint64_t)mcfg + sizeof(MCFGHeader) + sizeof(ACPIDeviceConfig[i]));

            for(uint64_t bus = deviceConfig->startBus; bus < deviceConfig->endBus; bus++)
            {
                EnumerateBus(deviceConfig->baseAddress, bus);
            }
        }
    }
}

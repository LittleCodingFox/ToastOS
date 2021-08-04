#include "PCI.hpp"
#include "debug.hpp"
#include "cstring/cstring.hpp"
#include "paging/PageTableManager.hpp"
#include "../drivers/AHCI/AHCIDriver.hpp"

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
    const char* deviceClasses[]
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

    const char* vendorName(uint16_t vendorID)
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

    const char* deviceName(uint16_t vendorID, uint16_t deviceID)
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

    const char* massStorageControllerSubclassName(uint8_t subclassCode)
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

    const char* serialBusControllerSubclassName(uint8_t subclassCode)
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

    const char* bridgeDeviceSubclassName(uint8_t subclassCode)
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

    const char* subclassName(uint8_t classCode, uint8_t subclassCode)
    {
        switch (classCode)
        {
            case 0x01:
                return massStorageControllerSubclassName(subclassCode);

            case 0x03:
                switch (subclassCode)
                {
                    case 0x00:
                        return "VGA Compatible Controller";
                }

            case 0x06:
                return bridgeDeviceSubclassName(subclassCode);

            case 0x0C:
                return serialBusControllerSubclassName(subclassCode);
        }
        return to_hstring(subclassCode);
    }

    const char* progIFName(uint8_t classCode, uint8_t subclassCode, uint8_t progIF)
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

    void enumerateFunction(uint64_t deviceAddress, uint64_t function)
    {
        uint64_t offset = function << 12;

        uint64_t functionAddress = deviceAddress + offset;

        globalPageTableManager->identityMap((void *)functionAddress);

        volatile PCIDeviceHeader *deviceHeader = (volatile PCIDeviceHeader *)functionAddress;

        if(deviceHeader->deviceID == 0 || deviceHeader->deviceID == 0xFFFF)
        {
            return;
        }

        printf("[PCI DEVICE]\n\tVendor: %s\n\tDevice Name: %s\n\tDevice Class: %s\n\tSubclass Name: %s\n\tProgIf Name: %s\n",
            vendorName(deviceHeader->vendorID),
            deviceName(deviceHeader->vendorID, deviceHeader->deviceID),
            deviceClasses[deviceHeader->objectClass],
            subclassName(deviceHeader->objectClass, deviceHeader->objectSubclass),
            progIFName(deviceHeader->objectClass, deviceHeader->objectSubclass, deviceHeader->progIF));

        Device *device = new Device();
        device->classCode = deviceHeader->objectClass;
        device->deviceID = deviceHeader->deviceID;
        device->vendorID = deviceHeader->vendorID;
        device->bars[0].address = ((volatile PCIHeader0 *)deviceHeader)->BAR0 & 0xFFFFFFF0;
        device->bars[1].address = ((volatile PCIHeader0 *)deviceHeader)->BAR1 & 0xFFFFFFF0;
        device->bars[2].address = ((volatile PCIHeader0 *)deviceHeader)->BAR2 & 0xFFFFFFF0;
        device->bars[3].address = ((volatile PCIHeader0 *)deviceHeader)->BAR3 & 0xFFFFFFF0;
        device->bars[4].address = ((volatile PCIHeader0 *)deviceHeader)->BAR4 & 0xFFFFFFF0;
        device->bars[5].address = ((volatile PCIHeader0 *)deviceHeader)->BAR5 & 0xFFFFFFF0;

        switch(deviceHeader->objectClass)
        {
            case 0x01: //Mass Storage Controller
                switch(deviceHeader->objectSubclass)
                {
                    case 0x06: //SATA
                        switch(deviceHeader->progIF)
                        {
                            case 0x01:
                                new Drivers::AHCI::AHCIDriver(device);

                            break;
                        }

                        break;
                }

                break;
        }
    }

    void enumerateDevice(uint64_t busAddress, uint64_t device)
    {
        uint64_t offset = device << 15;

        uint64_t deviceAddress = busAddress + offset;

        globalPageTableManager->identityMap((void *)deviceAddress);

        volatile PCIDeviceHeader *deviceHeader = (volatile PCIDeviceHeader *)deviceAddress;

        if(deviceHeader->deviceID == 0 || deviceHeader->deviceID == 0xFFFF)
        {
            return;
        }

        for(uint64_t i = 0; i < 8; i++)
        {
            enumerateFunction(deviceAddress, i);
        }
    }

    void enumerateBus(uint64_t baseAddress, uint64_t bus)
    {
        uint64_t offset = bus << 20;

        uint64_t busAddress = baseAddress + offset;

        globalPageTableManager->identityMap((void *)busAddress);

        volatile PCIDeviceHeader *deviceHeader = (volatile PCIDeviceHeader *)busAddress;

        if(deviceHeader->deviceID == 0 || deviceHeader->deviceID == 0xFFFF)
        {
            return;
        }

        for(uint64_t i = 0; i < 32; i++)
        {
            enumerateDevice(busAddress, i);
        }
    }

    void enumeratePCI(volatile MCFGHeader *mcfg)
    {
        printf("[PCI] Enumerating Devices\n");

        uint32_t entries = (mcfg->header.length - sizeof(MCFGHeader)) / sizeof(ACPIDeviceConfig);

        for(uint32_t i = 0; i < entries; i++)
        {
            volatile ACPIDeviceConfig *deviceConfig = (volatile ACPIDeviceConfig *)((uint64_t)mcfg + sizeof(MCFGHeader) + sizeof(ACPIDeviceConfig[i]));

            for(uint64_t bus = deviceConfig->startBus; bus < deviceConfig->endBus; bus++)
            {
                enumerateBus(deviceConfig->baseAddress, bus);
            }
        }
    }
}

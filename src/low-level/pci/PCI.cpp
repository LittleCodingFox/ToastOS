#include "PCI.hpp"
#include "debug.hpp"
#include "paging/PageTableManager.hpp"

namespace PCI
{
    void enumerateFunction(uint64_t deviceAddress, uint64_t function)
    {
        uint64_t offset = function << 12;

        uint64_t functionAddress = deviceAddress + offset;

        globalPageTableManager->identityMap((void *)functionAddress);

        PCIDeviceHeader *deviceHeader = (PCIDeviceHeader *)functionAddress;

        if(deviceHeader->deviceID == 0 || deviceHeader->deviceID == 0xFFFF)
        {
            return;
        }

        DEBUG_OUT("%x %x", deviceHeader->vendorID, deviceHeader->deviceID);
    }

    void enumerateDevice(uint64_t busAddress, uint64_t device)
    {
        uint64_t offset = device << 15;

        uint64_t deviceAddress = busAddress + offset;

        globalPageTableManager->identityMap((void *)deviceAddress);

        PCIDeviceHeader *deviceHeader = (PCIDeviceHeader *)deviceAddress;

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

        PCIDeviceHeader *deviceHeader = (PCIDeviceHeader *)busAddress;

        if(deviceHeader->deviceID == 0 || deviceHeader->deviceID == 0xFFFF)
        {
            return;
        }

        for(uint64_t i = 0; i < 32; i++)
        {
            enumerateDevice(busAddress, i);
        }
    }

    void enumeratePCI(MCFGHeader *mcfg)
    {
        uint32_t entries = (mcfg->header.length - sizeof(MCFGHeader)) / sizeof(ACPIDeviceConfig);

        for(uint32_t i = 0; i < entries; i++)
        {
            ACPIDeviceConfig *deviceConfig = (ACPIDeviceConfig *)((uint64_t)mcfg + sizeof(MCFGHeader) + sizeof(ACPIDeviceConfig[i]));

            for(uint64_t bus = deviceConfig->startBus; bus < deviceConfig->endBus; bus++)
            {
                enumerateBus(deviceConfig->baseAddress, bus);
            }
        }
    }
}

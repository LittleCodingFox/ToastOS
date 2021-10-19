#include "DeviceManager.hpp"
#include "debug.hpp"

namespace Devices
{
    DeviceManager globalDeviceManager;

    const char *GenericDevice::name() const
    {
        return "Generic Device";
    }

    DeviceType GenericDevice::type() const
    {
        return DEVICE_TYPE_UNKNOWN;
    }

    DeviceManager::DeviceManager() : deviceCount(0)
    {
    }

    void DeviceManager::AddDevice(GenericDevice *device)
    {
        if(deviceCount < MAX_DEVICES)
        {
            device->ID = deviceCount;
            devices[deviceCount++] = device;

            printf("Added device 0x%p (name: %s, type: 0x%x) with ID %u\n", device, device->name(), device->type(), device->ID);
        }
    }

    uint32_t DeviceManager::DeviceCount() const
    {
        return deviceCount;
    }

    GenericDevice *DeviceManager::GetDevice(uint32_t ID) const
    {
        if(ID < MAX_DEVICES)
        {
            return devices[ID];
        }

        return NULL;
    }

    frg::vector<GenericDevice *, frg_allocator> DeviceManager::GetDevices(DeviceType type)
    {
        frg::vector<GenericDevice *, frg_allocator> outValue;

        printf("Getting devices with type %x from %u devices\n", type, deviceCount);

        for(uint32_t i = 0; i < deviceCount; i++)
        {
            if(devices[i] != NULL && devices[i]->type() == type)
            {
                outValue.push_back(devices[i]);
            }
        }

        return outValue;
    }
}
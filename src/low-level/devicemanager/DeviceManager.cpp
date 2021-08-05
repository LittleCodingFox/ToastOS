#include "DeviceManager.hpp"

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
            return devices[MAX_DEVICES];
        }

        return NULL;
    }

    DynamicArray<GenericDevice *> DeviceManager::GetDevices(DeviceType type)
    {
        DynamicArray<GenericDevice *> outValue;

        for(uint32_t i = 0; i < deviceCount; i++)
        {
            if(devices[i] != NULL && devices[i]->type() == type)
            {
                outValue.add(devices[i]);
            }
        }

        return outValue;
    }
}
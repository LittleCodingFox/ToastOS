#pragma once
#include <stdint.h>
#include <new>
#include "kernel.h"

#define MAX_DEVICES 256

enum DeviceType
{
    DEVICE_TYPE_UNKNOWN,
    DEVICE_TYPE_DISK,
    DEVICE_TYPE_NETWORK
};

class GenericDevice
{
public:
    virtual const char *name() const;
    virtual DeviceType type() const;
    virtual ~GenericDevice() = default;
    uint32_t ID;
};

class DeviceManager
{
private:
    GenericDevice *devices[MAX_DEVICES];
    uint32_t deviceCount;
public:
    DeviceManager();
    void AddDevice(GenericDevice *device);
    uint32_t DeviceCount() const;
    vector<GenericDevice *> GetDevices(DeviceType type);
    GenericDevice *GetDevice(uint32_t ID) const;
};

extern DeviceManager globalDeviceManager;

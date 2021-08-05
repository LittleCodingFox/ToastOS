#pragma once
#include "DeviceManager.hpp"

namespace Devices
{
    class GenericIODevice : public GenericDevice
    {
    public:
        virtual bool Read(void *data, uint64_t sector, uint64_t count);
        virtual bool Write(void *data, uint64_t sector, uint64_t count);
        
        bool ReadUnaligned(void *data, uint64_t sector, uint64_t count);
        bool WriteUnaligned(void *data, uint64_t sector, uint64_t count);
    };
}
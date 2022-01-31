#pragma once
#include "DeviceManager.hpp"

class GenericIODevice : public GenericDevice
{
public:
    virtual bool Read(void *data, uint64_t sector, uint64_t count) = 0;
    virtual bool Write(const void *data, uint64_t sector, uint64_t count) = 0;
    
    bool ReadUnaligned(void *data, uint64_t sector, uint64_t count);
    bool WriteUnaligned(const void *data, uint64_t sector, uint64_t count);
};

#pragma once

#include "kernel.h"
#include "threading/lock.hpp"

class DiskCache
{
public:
    struct OwnerInfo;
private:
    struct CacheInfo
    {
        void *buffer;
        OwnerInfo *owner;

        CacheInfo() : buffer(NULL), owner(NULL) {};
    };

    frg::hash<uint64_t> hash;
    hashmap<uint64_t, CacheInfo> cache;
    AtomicLock lock;
public:
    struct OwnerInfo
    {
        uint8_t *buffer;
        int referenceCount;

        OwnerInfo() : buffer(NULL), referenceCount(0) {};
    };

    DiskCache() : cache(hash) {}

    void *Get(uint64_t sector);
    void Set(uint64_t sector, void *buffer, OwnerInfo *owner);
    void Remove(uint64_t sector);
};

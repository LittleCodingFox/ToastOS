#include "DiskCache.hpp"

void *DiskCache::Get(uint64_t sector)
{
    auto item = cache.get(sector);

    if(item == NULL)
    {
        return NULL;
    }

    return item->buffer;
}

void DiskCache::Set(uint64_t sector, void *buffer, OwnerInfo *owner)
{
    Remove(sector);

    CacheInfo info;
    info.buffer = buffer;
    info.owner = owner;

    owner->referenceCount++;

    cache.insert(sector, info);
}

void DiskCache::Remove(uint64_t sector)
{
    auto item = cache.get(sector);

    if(item == NULL)
    {
        return;
    }

    item->owner->referenceCount--;

    if(item->owner->referenceCount == 0)
    {
        delete [] item->owner->buffer;
        delete item->owner;
    }

    cache.remove(sector);
}

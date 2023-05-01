#include "DiskCache.hpp"

void *DiskCache::Get(uint64_t sector)
{
    ScopedLock l(lock);

    auto item = cache.get(sector);

    if(item == NULL || item->owner == NULL)
    {
        return NULL;
    }

    return item->buffer;
}

void DiskCache::Set(uint64_t sector, void *buffer, OwnerInfo *owner)
{
    Remove(sector);

    ScopedLock l(lock);
    
    CacheInfo info;
    info.buffer = buffer;
    info.owner = owner;

    owner->referenceCount++;

    cache.insert(sector, info);
}

void DiskCache::Remove(uint64_t sector)
{
    ScopedLock l(lock);
    
    auto item = cache.get(sector);

    if(item == NULL)
    {
        return;
    }

    if(item->owner == NULL)
    {
        cache.remove(sector);

        return;
    }

    item->owner->referenceCount--;

    if(item->owner->referenceCount == 0)
    {
        delete [] item->owner->buffer;
        delete item->owner;

        item->owner = NULL;
    }

    cache.remove(sector);
}

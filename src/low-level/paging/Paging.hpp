#pragma once
#include <stdint.h>

namespace PagingFlag
{
    enum PagingFlag
    {
        Present = (1 << 0),
        ReadWrite = (1 << 1),
        UserAccessible = (1 << 2),
        WriteThrough = (1 << 3),
        CacheDisabled = (1 << 4),
        Accessed = (1 << 5),
        LargerPages = (1 << 6),
        NoExecute = (1 << 7),
    };
}

struct PageDirectoryEntry
{
    union
    {
        uint64_t packed;
        
        struct
        {
            uint8_t present : 1;
            uint8_t writable : 1;
            uint8_t userAccessible : 1;
            uint8_t writeThruCache : 1;
            uint8_t disableCache : 1;
            uint8_t accessed : 1;
            uint8_t dirty : 1;
            uint8_t hugePage : 1;
            uint8_t global : 1;
            uint8_t OS1 : 1;
            uint8_t OS2 : 1;
            uint8_t OS3 : 1;
            uint64_t address : 40;
            uint8_t OS4 : 1;
            uint8_t OS5 : 1;
            uint8_t OS6 : 1;
            uint8_t OS7 : 1;
            uint8_t OS8 : 1;
            uint8_t OS9 : 1;
            uint8_t OSA : 1;
            uint8_t OSB : 1;
            uint8_t OSC : 1;
            uint8_t OSD : 1;
            uint8_t OSE : 1;
            uint8_t noExecute : 1;
        };
    };

    void SetAddress(uint64_t address);
    uint64_t GetAddress();
};

struct PageTable
{
    PageDirectoryEntry entries[512];
}__attribute__((aligned(0x1000)));

#pragma once
#include <stdint.h>

enum PT_Flag
{
    Present = 0,
    ReadWrite = 1,
    UserSuper = 2,
    WriteThrough = 3,
    CacheDisabled = 4,
    Accessed = 5,
    LargerPages = 7,
    Custom0 = 9,
    Custom1 = 10,
    Custom2 = 11,
    NX = 63 // only if supported
};

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

    void setAddress(uint64_t address);
    uint64_t getAddress();
};

struct PageTable
{
    PageDirectoryEntry entries[512];
}__attribute__((aligned(0x1000)));

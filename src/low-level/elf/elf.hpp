#pragma once

#include <stdint.h>
#include "paging/PageTableManager.hpp"

namespace Elf
{
    #define ELF_TYPE_EXECUTABLE             0x2

    #define ELF_PROGRAM_TYPE_LOAD           0x1
    #define ELF_PROGRAM_TYPE_INTERP         0x03
    #define ELF_PROGRAM_TYPE_PHDR           0x06

    #define ELF_SECTION_TYPE_NULL           0x0
    #define ELF_SECTION_TYPE_PROGBITS       0x1
    #define ELF_SECTION_TYPE_SYMTAB         0x2
    #define ELF_SECTION_TYPE_STRTAB         0x3
    #define ELF_SECTION_TYPE_NOBITS         0x8

    #define ELF_PROGRAM_FLAG_EXECUTE        0x1
    #define ELF_PROGRAM_FLAG_WRITE          0x2
    #define ELF_PROGRAM_FLAG_READ           0x4

    #define ELF_SECTION_FLAG_ALLOC          0x2

    #define AT_ENTRY                        10
    #define AT_PHDR                         20
    #define AT_PHENT                        21
    #define AT_PHNUM                        22

    struct __attribute__((packed)) ElfHeader
    {
        uint8_t identity[16];
        uint16_t type;
        uint16_t machine;
        uint32_t version;
        uint64_t entry;
        uint64_t phOffset;
        uint64_t shOffset;
        uint32_t flags;
        uint16_t headerSize;
        uint16_t phSize;
        uint16_t phNum;
        uint16_t shSize;
        uint16_t shNum;
        uint16_t strtabIndex;
    };

    struct __attribute__((packed)) ElfProgramHeader
    {
        uint32_t type;
        uint32_t flags;
        uint64_t offset;
        uint64_t virtualAddress;
        uint64_t physicalAddress;
        uint64_t fileSize;
        uint64_t memorySize;
        uint64_t alignment;
    };

    struct __attribute__((packed)) ElfSectionHeader
    {
        uint32_t name;
        uint32_t type;
        uint64_t flags;
        uint64_t virtualAddress;
        uint64_t offset;
        uint64_t size;
        uint32_t link;
        uint32_t info;
        uint64_t addressAlignment;
        uint64_t entrySize;
    };

    struct Auxval
    {
        uint64_t entry;
        uint64_t phdr;
        uint64_t programHeaderSize;
        uint64_t phNum;
    };

    ElfHeader *LoadElf(const void *data, uint64_t base, Auxval *auxval);
    void UnloadElf(ElfHeader *header);
    void MapElfSegments(ElfHeader *header, PageTableManager *pageTableManager, uint64_t base, Auxval *auxval, char **ldPath);
};

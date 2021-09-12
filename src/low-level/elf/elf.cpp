#include "elf.hpp"
#include "debug.hpp"
#include "paging/PageTableManager.hpp"
#include "paging/PageFrameAllocator.hpp"
#include <string.h>

namespace Elf
{
    void LoadElfSegment(const void *data, ElfProgramHeader *programHeader)
    {
        uint64_t memSize = programHeader->memorySize;
        uint64_t fileSize = programHeader->fileSize;
        uint64_t address = programHeader->virtualAddress;
        uint64_t offset = programHeader->offset;

        if(memSize == 0)
        {
            return;
        }

        uint32_t flags = PAGING_FLAG_PRESENT | PAGING_FLAG_USER_ACCESSIBLE;

        if(!(programHeader->flags & ELF_PROGRAM_FLAG_EXECUTE))
        {
            flags |= PAGING_FLAG_NO_EXECUTE;
        }

        if((programHeader->flags & ELF_PROGRAM_FLAG_WRITE))
        {
            flags |= PAGING_FLAG_WRITABLE;
        }

        uint64_t startPage = address / 0x1000;
        uint64_t pageCount = ((address + memSize) / 0x1000) - (address / 0x1000) + 1;

        DEBUG_OUT("Load at address %p with %llu pages (memsize: %llu; fileSize: %llu)", address, pageCount, memSize, fileSize);

        void *pages[pageCount];

        for(uint64_t i = 0; i < pageCount; i++)
        {
            void *physicalAddress = globalAllocator.RequestPage();
            pages[i] = physicalAddress;

            globalPageTableManager->MapMemory((void *)((startPage + i) * 0x1000), physicalAddress, flags | PAGING_FLAG_WRITABLE);
        }

        memcpy((void *)address, (uint8_t *)data + offset, fileSize);
        memset((void *)(address + fileSize), 0, memSize - fileSize);

        for(uint64_t i = 0; i < pageCount; i++)
        {
            globalPageTableManager->MapMemory((void *)((startPage + i) * 0x1000), pages[i], flags);
        }
    }

    ElfHeader *LoadElf(const void *data)
    {
        DEBUG_OUT("Loading Elf at ptr %p", data);

        if(data == NULL)
        {
            return NULL;
        }

        ElfHeader *header = (ElfHeader *)data;

        if(header->identity[0] != 0x7F ||
            header->identity[1] != 'E' ||
            header->identity[2] != 'L' ||
            header->identity[3] != 'F')
        {
            return NULL;            
        }

        if(header->headerSize != sizeof(ElfHeader))
        {
            DEBUG_OUT("%s", "[ELF] Invalid elf header size");

            return NULL;
        }
        else if(header->phSize != sizeof(ElfProgramHeader))
        {
            DEBUG_OUT("%s", "[ELF] Invalid program header size");

            return NULL;
        }

        uint16_t type = header->type;

        ElfProgramHeader *programHeader = (ElfProgramHeader *)((uint64_t)data + header->phOffset);

        for(uint64_t i = 0; i < header->phNum; i++)
        {
            if(programHeader[i].type == ELF_PROGRAM_TYPE_LOAD)
            {
                LoadElfSegment(data, &programHeader[i]);
            }
        }

        return header;
    }

    void UnloadElf(ElfHeader *header)
    {
        ElfProgramHeader *programHeader = (ElfProgramHeader *)((uint64_t)header + header->phOffset);

        for(uint64_t i = 0; i < header->phNum; i++)
        {
            if(programHeader[i].type == ELF_PROGRAM_TYPE_LOAD)
            {
                uint64_t memSize = programHeader[i].memorySize;
                uint64_t address = programHeader[i].virtualAddress;

                if(memSize == 0)
                {
                    continue;
                }

                uint64_t startPage = address / 0x1000;
                uint64_t pageCount = ((address + memSize) / 0x1000) - (address / 0x1000) + 1;

                globalAllocator.FreePages((void *)startPage, pageCount);

                for(uint64_t j = 0; j < pageCount; j++)
                {
                    globalPageTableManager->UnmapMemory((void *)((startPage + j) * 0x1000));
                }
            }
        }
    }
}
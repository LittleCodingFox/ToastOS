#include <string.h>
#include "elf.hpp"
#include "debug.hpp"
#include "paging/PageTableManager.hpp"
#include "paging/PageFrameAllocator.hpp"
#include "registers/Registers.hpp"

namespace Elf
{
    bool LoadElfSegment(const void *data, ElfProgramHeader *programHeader, PageTableManager *pageTableManager, uint64_t base)
    {
        uint64_t memSize = programHeader->memorySize;
        uint64_t fileSize = programHeader->fileSize;
        uint64_t address = programHeader->virtualAddress + base;
        uint64_t offset = programHeader->offset;

        if(memSize == 0)
        {
            return false;
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

#if DEBUG_PROCESSES
        DEBUG_OUT("Load at address %p with %lu pages (%p-%p) (memsize: %lu; fileSize: %lu; offset: %p)",
            (void *)address,
            pageCount,
            (void *)((address / 0x1000) * 0x1000),
            (void *)(((address / 0x1000) + pageCount) * 0x1000),
            memSize, fileSize,
            (void *)offset);
#endif

        auto cr3 = Registers::ReadCR3();

        PageTableManager localPageTable;
        localPageTable.p4 = (PageTable *)cr3;

        auto physical = (uint64_t)globalAllocator.RequestPages(pageCount);
        auto higherPhysical = TranslateToHighHalfMemoryAddress(physical);

        for(uint64_t i = 0; i < pageCount; i++)
        {
            uint64_t physicalAddress = (physical + i * 0x1000);
            uint64_t userAddress = (startPage + i) * 0x1000;
            uint64_t localPhysicalAddress = TranslateToHighHalfMemoryAddress(physicalAddress);

            pageTableManager->MapMemory((void *)userAddress, (void *)physicalAddress, flags);
            localPageTable.MapMemory((void *)localPhysicalAddress, (void *)physicalAddress, flags | PAGING_FLAG_WRITABLE);
        }

        memset((void *)higherPhysical, 0, pageCount * 0x1000);
        memcpy((void *)(higherPhysical + address % 0x1000), (uint8_t *)data + offset, fileSize);

        return true;
    }

    ElfHeader *LoadElf(const void *data, uint64_t base, Auxval *auxval)
    {
#if DEBUG_PROCESSES
        DEBUG_OUT("Loading Elf at ptr %p", data);
#endif

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
#if DEBUG_PROCESSES
            DEBUG_OUT("%s", "[ELF] Invalid elf header size");
#endif

            return NULL;
        }
        else if(header->phSize != sizeof(ElfProgramHeader))
        {
#if DEBUG_PROCESSES
            DEBUG_OUT("%s", "[ELF] Invalid program header size");
#endif

            return NULL;
        }

        Auxval value;

        value.entry = base + header->entry;
        value.phdr = 0;
        value.programHeaderSize = sizeof(ElfProgramHeader);
        value.phNum = header->phNum;

        *auxval = value;

        return header;
    }

    void MapElfSegments(ElfHeader *header, PageTableManager *pageTableManager, uint64_t base, Auxval *auxval, char **ldPath)
    {
        *ldPath = NULL;

        ElfProgramHeader *programHeader = (ElfProgramHeader *)((uint64_t)header + header->phOffset);

        for(uint64_t i = 0; i < header->phNum; i++)
        {
            ElfProgramHeader *currentHeader = &programHeader[i];

            if(currentHeader->type == ELF_PROGRAM_TYPE_LOAD)
            {
                LoadElfSegment(header, currentHeader, pageTableManager, base);
            }
            else if(currentHeader->type == ELF_PROGRAM_TYPE_PHDR)
            {
                auxval->phdr = base + currentHeader->virtualAddress;

#if DEBUG_PROCESSES
                DEBUG_OUT("PHDR: %p", (void *)auxval->phdr);
#endif
            }
            else if(currentHeader->type == ELF_PROGRAM_TYPE_INTERP)
            {
                uint32_t fileSize = currentHeader->fileSize;
                char *fileName = new char[fileSize + 1];

                fileName[fileSize] = '\0';

                memcpy(fileName, (uint8_t *)header + currentHeader->offset, fileSize);

                *ldPath = fileName;
            }
        }
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
                    //globalPageTableManager->UnmapMemory((void *)(TranslateToHighHalfMemoryAddress((startPage + j) * 0x1000)));
                }
            }
        }
    }
}
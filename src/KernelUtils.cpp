#include <string.h>
#include "KernelUtils.hpp"
#include "gdt/gdt.hpp"
#include "interrupts/IDT.hpp"
#include "interrupts/Interrupts.hpp"
#include "ports/Ports.hpp"
#include "registers/Registers.hpp"
#include "stacktrace/stacktrace.hpp"
#include "timer/Timer.hpp"
#include "pci/PCI.hpp"
#include "vtconsole/vtconsole.h"

KernelInfo kernelInfo; 
PageTableManager pageTableManager = NULL;

void InitializeMemory(BootInfo* bootInfo)
{
    uint64_t mMapEntries = bootInfo->mMapSize / bootInfo->mMapDescSize;

    globalAllocator = PageFrameAllocator();
    globalAllocator.readEFIMemoryMap(bootInfo->mMap, bootInfo->mMapSize, bootInfo->mMapDescSize);

    uint64_t kernelSize = (uint64_t)&_KernelEnd - (uint64_t)&_KernelStart;
    uint64_t kernelPages = (uint64_t)kernelSize / 4096 + 1;

    DEBUG_OUT("%s", "Locking kernel pages");

    globalAllocator.lockPages(&_KernelStart, kernelPages);

    PageTable* PML4 = (PageTable*)globalAllocator.requestPage();
    memset(PML4, 0, 0x1000);

    DEBUG_OUT("%s", "Initializing page table manager");

    pageTableManager = PageTableManager(PML4);

    //Enable write protection
    Registers::writeCR0(Registers::readCR0() | (1 << 16));

    //Enable NXE bit
    uint64_t efer = Registers::readMSR(Registers::IA32_EFER);
    Registers::writeMSR(Registers::IA32_EFER, efer | (1 << 11));

    //Enables memory-wide identity mapping. probably not a good thing.
    for (uint64_t t = 0; t < getMemorySize(bootInfo->mMap, mMapEntries, bootInfo->mMapDescSize); t+= 0x1000)
    {
        pageTableManager.identityMap((void*)t);
    }

    //TODO: Make it map only specific memory
    /*
    for (uint64_t t = _KernelStart; t < _KernelStart + kernelSize; t+= 0x1000)
    {
        pageTableManager.identityMap((void*)t);
    }
    */

    DEBUG_OUT("%s", "Preparing framebuffer pages");

    uint64_t fbBase = (uint64_t)bootInfo->framebuffer->baseAddress;
    uint64_t fbSize = (uint64_t)bootInfo->framebuffer->bufferSize + 0x1000;

    globalAllocator.lockPages((void*)fbBase, fbSize / 0x1000 + 1);

    for (uint64_t t = fbBase; t < fbBase + fbSize; t += 4096)
    {
        pageTableManager.identityMap((void*)t);
    }

    Registers::writeCR3((uint64_t)PML4);

    globalPageTableManager = kernelInfo.pageTableManager = &pageTableManager;
}

void InitializeInterrupts()
{
    printf("Initializing Interrupts\n");

    interrupts.init();
}

void InitializeACPI(BootInfo *bootInfo)
{
    printf("Initializing ACPI\n");

    if(bootInfo->rsdp == NULL)
    {
        printf("[ACPI] Missing RSDP!\n");

        return;
    }

    char signature[9] = { 0 };

    char OEMID[7] = { 0 };

    memcpy(signature, bootInfo->rsdp->signature, 8);
    memcpy(OEMID, bootInfo->rsdp->OEMID, 6);

    printf("[ACPI] RSDP Signature: %s\n[ACPI] OEMID: %s\n", signature, OEMID);

    printf("[ACPI] Length: %u\n[ACPI] RSDT Address: %08x\n[ACPI] XSDT Address: %p\n",
        bootInfo->rsdp->length, bootInfo->rsdp->RSDTAddress,
        bootInfo->rsdp->XSDTAddress);

    SDTHeader *xsdt = (SDTHeader *)bootInfo->rsdp->XSDTAddress;

    printf("[ACPI] Got XSDT: %p\n", xsdt);

    if(xsdt == NULL)
    {
        printf("[ACPI] Failed to get XSDT!\n");

        return;
    }

    MCFGHeader *mcfg = (MCFGHeader *)ACPI::findTable(xsdt, "MCFG");

    printf("[ACPI] Got MCFG: %p\n", mcfg);

    if(mcfg == NULL)
    {
        printf("[ACPI] Failed to get MCFG!");

        return;
    }

    PCI::enumeratePCI(mcfg);
}

FramebufferRenderer r = FramebufferRenderer(NULL, NULL);

vtconsole_t *console = NULL;

void PaintHandler(struct vtconsole* vtc, vtcell_t* cell, int x, int y)
{
    globalRenderer->putChar(cell->c, x * globalRenderer->fontWidth(), y * globalRenderer->fontHeight());
}

void CursorHandler (struct vtconsole* vtc, vtcursor_t* cur)
{
}

void RefreshFramebuffer();

KernelInfo InitializeKernel(BootInfo* bootInfo)
{
    kernelInitStacktrace(bootInfo->symbols, bootInfo->symbolsSize);

    r = FramebufferRenderer(bootInfo->framebuffer, bootInfo->font);

    globalRenderer = &r;

    GDTDescriptor gdtDescriptor;

    gdtDescriptor.size = sizeof(GDT) - 1;
    gdtDescriptor.offset = (uint64_t)&DefaultGDT;

    LoadGDT(&gdtDescriptor);

    InitializeMemory(bootInfo);

    globalRenderer->initialize();

    int consoleWidth = globalRenderer->width() / globalRenderer->fontWidth();
    int consoleHeight = globalRenderer->height() / globalRenderer->fontHeight();

    DEBUG_OUT("creating console: width: %d; height: %d;", consoleWidth, consoleHeight);

    console = vtconsole(consoleWidth, consoleHeight, PaintHandler, CursorHandler);

    InitializeInterrupts();

    timer.initialize();

    timer.registerHandler(RefreshFramebuffer);

    InitializeACPI(bootInfo);

    DEBUG_OUT("%s", "Finished initializing the kernel");

    double MBSize = 1024 * 1024;

    DEBUG_OUT("Memory Stats: Free: %.2lfMB; Used: %.2lfMB; Reserved: %.2lfMB", globalAllocator.getFreeRAM() / MBSize,
        globalAllocator.getUsedRAM() / MBSize, globalAllocator.getReservedRAM() / MBSize);

    return kernelInfo;
}

void _putchar(char character)
{
    vtconsole_putchar(console, character);
    SerialPortOutStreamCOM1(character, NULL);
}

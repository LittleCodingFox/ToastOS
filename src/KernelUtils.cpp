#include <string.h>
#include "KernelUtils.hpp"
#include "gdt/gdt.hpp"
#include "interrupts/IDT.hpp"
#include "interrupts/Interrupts.hpp"
#include "ports/Ports.hpp"
#include "registers/Registers.hpp"

KernelInfo kernelInfo; 
PageTableManager pageTableManager = NULL;

void PrepareMemory(BootInfo* bootInfo)
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
        pageTableManager.mapMemory((void*)t, (void*)t);
    }

    //TODO: Make it map only specific memory
    /*
    for (uint64_t t = _KernelStart; t < _KernelStart + kernelSize; t+= 0x1000)
    {
        pageTableManager.mapMemory((void*)t, (void*)t);
    }
    */

    DEBUG_OUT("%s", "Preparing framebuffer pages");

    uint64_t fbBase = (uint64_t)bootInfo->framebuffer->baseAddress;
    uint64_t fbSize = (uint64_t)bootInfo->framebuffer->bufferSize + 0x1000;

    globalAllocator.lockPages((void*)fbBase, fbSize/ 0x1000 + 1);

    for (uint64_t t = fbBase; t < fbBase + fbSize; t += 4096)
    {
        pageTableManager.mapMemory((void*)t, (void*)t);
    }

    Registers::writeCR3((uint64_t)PML4);

    globalPageTableManager = kernelInfo.pageTableManager = &pageTableManager;

    DEBUG_OUT("%s", "Finished initializing the kernel");
}

void PrepareInterrupts()
{
    idt.init();
    idt.registerInterrupt(0xE, (uint64_t)PageFault_Handler);
    idt.registerInterrupt(0x8, (uint64_t)DoubleFault_Handler);
    idt.registerInterrupt(0xD, (uint64_t)GPFault_Handler);
    idt.registerInterrupt(0x21, (uint64_t)KeyboardInt_Handler);

    RemapPIC();

    outport8(PIC1_DATA, 0b11111101);
    outport8(PIC2_DATA, 0b11111111);

    asm ("sti");
}

FramebufferRenderer r = FramebufferRenderer(NULL, NULL);

KernelInfo InitializeKernel(BootInfo* bootInfo)
{
    r = FramebufferRenderer(bootInfo->framebuffer, bootInfo->font);

    globalRenderer = &r;

    GDTDescriptor gdtDescriptor;

    gdtDescriptor.Size = sizeof(GDT) - 1;
    gdtDescriptor.Offset = (uint64_t)&DefaultGDT;

    LoadGDT(&gdtDescriptor);

    PrepareMemory(bootInfo);

    memset(bootInfo->framebuffer->baseAddress, 0, bootInfo->framebuffer->bufferSize);

    PrepareInterrupts();

    return kernelInfo;
}

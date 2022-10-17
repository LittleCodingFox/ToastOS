#include <string.h>
#include <lai/core.h>
#include <lai/helpers/sci.h>
#include <lai/helpers/pm.h>
#include "KernelUtils.hpp"
#include "tss/tss.hpp"
#include "gdt/gdt.hpp"
#include "interrupts/IDT.hpp"
#include "interrupts/Interrupts.hpp"
#include "ports/Ports.hpp"
#include "registers/Registers.hpp"
#include "stacktrace/stacktrace.hpp"
#include "pit/PIT.hpp"
#include "vtconsole/vtconsole.h"
#include "partitionmanager/PartitionManager.hpp"
#include "tss/tss.hpp"
#include "syscall/syscall.hpp"
#include "../klibc/sys/syscall.h"
#include "process/Process.hpp"
#include "schedulers/RoundRobinScheduler.hpp"
#include "partitionmanager/PartitionManager.hpp"
#include "filesystems/VFS.hpp"
#include "filesystems/tarfs/tarfs.hpp"
#include "sse/sse.hpp"
#include "cmos/cmos.hpp"
#include "keyboard/Keyboard.hpp"
#include "input/InputSystem.hpp"
#include "kasan/kasan.hpp"
#include "mouse/Mouse.hpp"
#include "pci/PCI.hpp"
#include "smp/SMP.hpp"
#include "drivers/AHCI/AHCIDriver.hpp"
#include "drivers/networking/RTL8139/RTL8139.hpp"
#include "madt/MADT.hpp"
#include "lapic/LAPIC.hpp"
#include "time/time.hpp"
#include "net/net.hpp"

PageTableManager pageTableManager;

using symbol = char[];

extern "C" symbol text_start_addr, text_end_addr,
    rodata_start_addr, rodata_end_addr,
    data_start_addr, data_end_addr;

limine_file *GetModule(volatile limine_module_request *modules, const char *name)
{
    for(uint64_t i = 0; i < modules->response->module_count; i++)
    {
        limine_file *module = modules->response->modules[i];

        if(strncmp(name, module->cmdline, 128) == 0)
        {
            return module;
        }
    }

    return NULL;
}

uint64_t GetMemorySize(volatile limine_memmap_request *memmap)
{
    static uint64_t memorySize = 0;

    if(memorySize > 0)
    {
        return memorySize;
    }

    for(uint64_t i = 0; i < memmap->response->entry_count; i++)
    {
        memorySize += memmap->response->entries[i]->length;
    }

    return memorySize;
}

void InitializeMemory(volatile limine_memmap_request *memmap, volatile limine_framebuffer_request *framebuffer,
    volatile limine_hhdm_request *hhdm, volatile limine_kernel_address_request *kernelAddress)
{
    globalAllocator.ReadMemoryMap(memmap, hhdm);

    DEBUG_OUT("%s", "Initializing page table manager");

    pageTableManager.p4 = (PageTable *)globalAllocator.RequestPage();

    pageTableManager.IdentityMap((void *)(uint64_t)pageTableManager.p4,
        PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);

    //Preallocate upper area
    for(uint64_t i = 256; i < 512; i++)
    {
        void *page = globalAllocator.RequestPage();
        memset(page, 0, 0x1000);

        pageTableManager.p4->entries[i] = (uint64_t)page | PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE;
    }

    //Enable write protection
    Registers::WriteCR0(Registers::ReadCR0() | (1 << 16));

    // Program the PAT. Each byte configures a single entry.
    // 00: Uncacheable
    // 01: Write Combining
    // 04: Write Through
    // 06: Write Back
    //Registers::WriteMSR(0x277, 0x00'00'01'00'00'00'04'06);
    Registers::WriteMSR(0x0277, 0x0000000005010406);

    DEBUG_OUT("%s", "Mapping memmap");

    #define ALIGN_DOWN(value, align) ((value / align) * align)
    #define ALIGN_UP(value, align) (((value + (align - 1)) / align) * align)

    uint64_t textStart = ALIGN_DOWN((uint64_t)text_start_addr, 0x1000);
    uint64_t textEnd = ALIGN_UP((uint64_t)text_end_addr, 0x1000);
    uint64_t rodataStart = ALIGN_DOWN((uint64_t)rodata_start_addr, 0x1000);
    uint64_t rodataEnd = ALIGN_UP((uint64_t)rodata_end_addr, 0x1000);
    uint64_t dataStart = ALIGN_DOWN((uint64_t)data_start_addr, 0x1000);
    uint64_t dataEnd = ALIGN_UP((uint64_t)data_end_addr, 0x1000);

    for(uint64_t textAddress = textStart; textAddress < textEnd; textAddress += 0x1000)
    {
        uint64_t target = textAddress - kernelAddress->response->virtual_base + kernelAddress->response->physical_base;

        pageTableManager.MapMemory((void *)textAddress, (void *)target, PAGING_FLAG_PRESENT);
    }

    for(uint64_t rodataAddress = rodataStart; rodataAddress < rodataEnd; rodataAddress += 0x1000)
    {
        uint64_t target = rodataAddress - kernelAddress->response->virtual_base + kernelAddress->response->physical_base;

        pageTableManager.MapMemory((void *)rodataAddress, (void *)target, PAGING_FLAG_PRESENT | PAGING_FLAG_NO_EXECUTE);
    }

    for(uint64_t dataAddress = dataStart; dataAddress < dataEnd; dataAddress += 0x1000)
    {
        uint64_t target = dataAddress - kernelAddress->response->virtual_base + kernelAddress->response->physical_base;

        pageTableManager.MapMemory((void *)dataAddress, (void *)target, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_NO_EXECUTE);
    }

    for(uint64_t address = 0x1000; address < 0x100000000; address += 0x1000)
    {
        pageTableManager.IdentityMap((void *)address, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
        pageTableManager.MapMemory((void *)TranslateToHighHalfMemoryAddress(address), (void *)address,
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_NO_EXECUTE);
    }

    for (uint64_t i = 0; i < memmap->response->entry_count; i++)
    {
        struct limine_memmap_entry *entry = memmap->response->entries[i];

        uint64_t base = ALIGN_DOWN(entry->base, 0x1000);
        uint64_t top = ALIGN_UP(entry->base + entry->length, 0x1000);

        if (top <= 0x100000000)
        {
            continue;
        }

        for (uint64_t j = base; j < top; j += 0x1000) {

            if (j < 0x100000000)
            {
                continue;
            }

            pageTableManager.IdentityMap((void *)j, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
            pageTableManager.MapMemory((void *)TranslateToHighHalfMemoryAddress(j), (void *)j,
                PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_NO_EXECUTE);
        }
    }

    DEBUG_OUT("Preparing framebuffer pages for address %p", framebuffer->response->framebuffers[0]->address);

    uint64_t fbBase = (uint64_t)framebuffer->response->framebuffers[0]->address;
    uint64_t fbSize = (uint64_t)framebuffer->response->framebuffers[0]->height * framebuffer->response->framebuffers[0]->pitch;

    DEBUG_OUT("Framebuffer width: %u; height: %u; bpp: %u; pitch: %u",
        framebuffer->response->framebuffers[0]->width, framebuffer->response->framebuffers[0]->height,
        framebuffer->response->framebuffers[0]->bpp, framebuffer->response->framebuffers[0]->pitch);

    DEBUG_OUT("fbBase: %p", fbBase);

    for (uint64_t t = fbBase; t < fbBase + fbSize; t += 0x1000)
    {
        pageTableManager.MapMemory((void *)t, (void *)TranslateToPhysicalMemoryAddress((uint64_t)t),
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_WRITE_COMBINE);
    }

    framebuffer->response->framebuffers[0]->address = (void *)fbBase;

    DEBUG_OUT("%s", "Initialized Framebuffer");

    pageTableManager.MapMemory((void *)TranslateToHighHalfMemoryAddress((uint64_t)globalAllocator.PageBitmap.buffer), (void *)globalAllocator.PageBitmap.buffer,
        PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);

    globalAllocator.PageBitmap.buffer = (uint8_t *)TranslateToHighHalfMemoryAddress((uint64_t)globalAllocator.PageBitmap.buffer);

    Registers::WriteCR3((uint64_t)pageTableManager.p4);

    DEBUG_OUT("Wrote CR3 (%p)", pageTableManager.p4);

    globalPageTableManager = &pageTableManager;

    DEBUG_OUT("%s", "Initialized memory");
}

void InitializeACPI(volatile limine_rsdp_request *rsdp)
{
    (void)rsdp;

    DEBUG_OUT("[ACPI] Initializing ACPI", 0);

    if(rsdp == NULL)
    {
        DEBUG_OUT("[ACPI] Missing RSDP!", 0);

        return;
    }

    volatile RSDPDescriptor *rsdpStruct = (volatile RSDPDescriptor *)rsdp->response->address;

    char signature[9] = { 0 };

    char OEMID[7] = { 0 };

    memcpy(signature, (const void *)rsdpStruct->signature, 8);
    memcpy(OEMID, (const void *)rsdpStruct->OEMID, 6);

    DEBUG_OUT("[ACPI] RSDP Signature: %s\n[ACPI] OEMID: %s\n[ACPI] Revision: %u",
        signature,
        OEMID,
        rsdpStruct->revision);

    if(rsdpStruct->revision == 0)
    {
        DEBUG_OUT("[ACPI] Unsupported ACPI revision", 0);

        return;
    }

    volatile RSDPDescriptor2 *rsdpStructReal = (volatile RSDPDescriptor2 *)rsdpStruct;

    if(rsdpStructReal->XSDTAddress == 0)
    {
        DEBUG_OUT("[ACPI] Failed to get XSDT!", 0);

        return;
    }

    xsdt = (volatile SDTHeader *)TranslateToHighHalfMemoryAddress(rsdpStructReal->XSDTAddress);

    ACPI::DumpTables(xsdt);

    mcfg = (volatile MCFGHeader *)ACPI::FindTable(xsdt, "MCFG");

    if(mcfg == NULL)
    {
        DEBUG_OUT("[ACPI] Failed to get MCFG!", 0);
    }

    madt = (volatile MADT *)ACPI::FindTable(xsdt, "APIC");

    lai_create_namespace();

    lai_enable_acpi(1);

    InitializeMADT();

    InitializePCI();

    PCIEnumerateDevices(0x8086, 0x2922, [](PCIDevice *device) {

        Drivers::AHCI::PrepareDevice(device);
    });

    PCIEnumerateDevices(0, 0, 0x0C, 0x03, 0x30, [](PCIDevice *device) {

        DEBUG_OUT("[pci] Found xHCI controller with vendor %04x and device %04x", device->Vendor(), device->Device());
    });

    Drivers::Networking::RTL8139Query();

    globalPartitionManager->Initialize();
}

vtconsole_t *console = NULL;

static uint32_t colors[] =
{
  [VTCOLOR_BLACK] = 0x00000000,
  [VTCOLOR_RED] = 0xFFFF0000,
  [VTCOLOR_GREEN] = 0xFF008000,
  [VTCOLOR_YELLOW] = 0xFFA52A2A,
  [VTCOLOR_BLUE] = 0xFF0000FF,
  [VTCOLOR_MAGENTA] = 0xFFFF00FF,
  [VTCOLOR_CYAN] = 0xFF00FFFF,
  [VTCOLOR_GREY] = 0xFFD3D3D3,
};

static uint32_t brightcolors[] =
{
  [VTCOLOR_BLACK] = 0xFFA9A9A9,
  [VTCOLOR_RED] = 0xFFFFCCCB,
  [VTCOLOR_GREEN] = 0xFF90EE90,
  [VTCOLOR_YELLOW] = 0xFFFFFF99,
  [VTCOLOR_BLUE] = 0xFFADD8E6,
  [VTCOLOR_MAGENTA] = 0xFFE78BE7,
  [VTCOLOR_CYAN] = 0xFFE0FFFF,
  [VTCOLOR_GREY] = 0xFFFFFF,
};

void PaintHandler(struct vtconsole* vtc, vtcell_t* cell, int x, int y)
{
    uint32_t color = cell->attr.bright ? brightcolors[cell->attr.fg] : colors[cell->attr.fg];

    globalRenderer->PutChar(cell->c, x * globalRenderer->FontWidth(), y * globalRenderer->FontHeight(), color);
}

void CursorHandler(struct vtconsole* vtc, vtcursor_t* cur)
{
}

void RefreshFramebuffer(InterruptStack *stack);

void InitializeKernel(volatile limine_framebuffer_request *framebuffer, volatile limine_memmap_request *memmap, volatile limine_module_request *modules,
    volatile limine_hhdm_request *hhdm, volatile limine_kernel_address_request *kernelAddress, volatile limine_smp_request *smp,
    volatile limine_rsdp_request *rsdp)
{
    InitializeSerial();

    limine_file *symbols = GetModule(modules, "symbols.map");
    limine_file *font = GetModule(modules, "font.psf");
#if USE_TARFS
    limine_file *initrd = GetModule(modules, "initrd");
#endif

    LoadGDT(&bootstrapGDT, &bootstrapTSS, bootstrapTssStack, bootstrapist1Stack, bootstrapist2Stack, sizeof(bootstrapTssStack), &bootstrapGDTR);

    DEBUG_OUT("[INTERRUPTS] Initializing Interrupts", 0);

    InitializeMemory(memmap, framebuffer, hhdm, kernelAddress);

    interrupts.Init();

#if USE_KASAN
    DEBUG_OUT("Kasan: Unpoison framebuffer", 0);
#endif

    UnpoisonKasanShadow((void *)TranslateToHighHalfMemoryAddress((uint64_t)globalPageTableManager->p4), 0x1000);

    UnpoisonKasanShadow((void *)framebuffer->response->framebuffers[0]->address,
        (uint64_t)framebuffer->response->framebuffers[0]->height * framebuffer->response->framebuffers[0]->pitch);

    //TODO: Fix Kasan unpoisoning after limine port
    /*
    for (uint64_t i = 0; i < memmap->response->entry_count; i++)
    {
        limine_memmap_entry* desc = (limine_memmap_entry*)memmap->response->entries[i];

        if(desc->type == STIVALE2_MMAP_KERNEL_AND_MODULES)
        {
            auto base = desc->base;

            if(TranslateToKernelMemoryAddress(desc->base) <= 0xFFFFFFFF90000000)
            {
                base = TranslateToKernelMemoryAddress(base);
            }
            else
            {
                base = TranslateToHighHalfMemoryAddress(base);
            }

#if USE_KASAN
            DEBUG_OUT("Kasan: Unpoison kernel/modules at %p (%p)", base, desc->base);
#endif

            UnpoisonKasanShadow((void *)base, desc->length);
        }
    }
    */

    if(symbols != NULL)
    {
        DEBUG_OUT("Initializing kernel symbols from size %llu", symbols->size);

        KernelInitStacktrace((char *)symbols->address, symbols->size);
    }

    EnableSSE();

    psf2_font_t *psf2Font = NULL;

    if(font != NULL)
    {
        psf2Font = psf2Load((void *)font->address);
    }

    Framebuffer *framebufferStruct = new Framebuffer();

    framebufferStruct->baseAddress = (void *)framebuffer->response->framebuffers[0]->address;
    framebufferStruct->bufferSize = framebuffer->response->framebuffers[0]->pitch * framebuffer->response->framebuffers[0]->height;
    framebufferStruct->height = framebuffer->response->framebuffers[0]->height;
    framebufferStruct->width = framebuffer->response->framebuffers[0]->width;
    framebufferStruct->pixelsPerScanLine = framebuffer->response->framebuffers[0]->pitch;

    globalRenderer = new FramebufferRenderer(framebufferStruct, psf2Font);

    globalRenderer->Initialize();

    int consoleWidth = globalRenderer->Width() / globalRenderer->FontWidth();
    int consoleHeight = globalRenderer->Height() / globalRenderer->FontHeight();

    DEBUG_OUT("creating console: width: %d; height: %d;", consoleWidth, consoleHeight);

    console = vtconsole(consoleWidth, consoleHeight, PaintHandler, CursorHandler);

    globalInputSystem.initialize();

    vfs.initialize();

    globalPartitionManager.initialize();

    DEBUG_OUT("[ACPI] Initializing acpi", 0);

    InitializeACPI(rsdp);

    DEBUG_OUT("[CMOS] Initializing cmos", 0);

    cmos.Initialize();

#if USE_TARFS
    if(initrd != NULL)
    {
        auto tarfs = new TarFS((uint8_t *)initrd->address);

        vfs->AddMountPoint("/", tarfs);

        //tarfs->DebugListDirectories();
    }
#endif

    InitializeVirtualFiles();

    InitializeKeyboard();

    InitializeMouse();

    processManager.initialize();

    DEBUG_OUT("%s", "Finished initializing the kernel");

    uint64_t MBSize = 1024 * 1024;

    (void)MBSize;

    DEBUG_OUT("Memory Stats: Free: %lluMB; Used: %lluMB; Reserved: %lluMB", globalAllocator.GetFreeRAM() / MBSize,
        globalAllocator.GetUsedRAM() / MBSize, globalAllocator.GetReservedRAM() / MBSize);

    if(smp != NULL)
    {
        InitializeSMP(smp);
    }

    InitializeLAPIC();

    InitializeTime();

    interrupts.EnableInterrupts();

    InitializeNetworking();
}

void _putchar(char character)
{
    if(console != NULL)
    {
        vtconsole_putchar(console, character);
    }
    
    SerialPortOutStreamCOM1(character, NULL);
}

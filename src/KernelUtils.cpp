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

PageTableManager pageTableManager;

void *Stivale2GetTag(stivale2_struct *stivale2Struct, uint64_t ID)
{
    stivale2_tag *current = (stivale2_tag *)stivale2Struct->tags;

    for(;;)
    {
        if(current == NULL)
        {
            return NULL;
        }

        if(current->identifier == ID)
        {
            return current;
        }

        current = (stivale2_tag *)current->next;
    }
}

stivale2_module *Stivale2GetModule(stivale2_struct_tag_modules *modules, const char *name)
{
    for(uint64_t i = 0; i < modules->module_count; i++)
    {
        stivale2_module *module = &modules->modules[i];

        if(strncmp(name, module->string, 128) == 0)
        {
            return module;
        }
    }

    return NULL;
}

uint64_t GetMemorySize(stivale2_struct_tag_memmap *memmap)
{
    static uint64_t memorySize = 0;

    if(memorySize > 0)
    {
        return memorySize;
    }

    for(uint64_t i = 0; i < memmap->entries; i++)
    {
        memorySize += memmap->memmap[i].length;
    }

    return memorySize;
}

void InitializeMemory(stivale2_struct_tag_memmap *memmap, stivale2_struct_tag_framebuffer *framebuffer)
{
    globalAllocator.ReadMemoryMap(memmap);

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

    /*
    //Enable write protection
    Registers::WriteCR0(Registers::ReadCR0() | (1 << 16));

    //Enable NXE bit
    uint64_t efer = Registers::ReadMSR(Registers::IA32_EFER);
    Registers::WriteMSR(Registers::IA32_EFER, efer | (1 << 11));
    */

    // Program the PAT. Each byte configures a single entry.
    // 00: Uncacheable
    // 01: Write Combining
    // 04: Write Through
    // 06: Write Back
    //Registers::WriteMSR(0x277, 0x00'00'01'00'00'00'04'06);
    Registers::WriteMSR(0x0277, 0x0000000005010406);

    DEBUG_OUT("%s", "Identity mapping the whole memory");

    for(uint64_t index = 0; index < GetMemorySize(memmap); index += 0x1000)
    {
        pageTableManager.MapMemory((void *)TranslateToHighHalfMemoryAddress(index), (void *)(index),
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
    }

    DEBUG_OUT("%s", "Mapping memmap");

    for (int i = 0; i < memmap->entries; i++)
    {
        stivale2_mmap_entry* desc = (stivale2_mmap_entry*)&memmap->memmap[i];

        if(desc->type == STIVALE2_MMAP_KERNEL_AND_MODULES)
        {
            for(uint64_t index = 0; index < desc->length / 0x1000 + 1; index++)
            {
                pageTableManager.MapMemory((void *)(HIGHER_HALF_KERNEL_MEMORY_OFFSET + desc->base + index * 0x1000), (void *)(desc->base + index * 0x1000),
                    PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
            }
        }
        else if(desc->type != STIVALE2_MMAP_USABLE)
        {
            for(uint64_t index = 0; index < desc->length / 0x1000 + 1; index++)
            {
                pageTableManager.MapMemory((void *)TranslateToHighHalfMemoryAddress(desc->base + index * 0x1000), (void *)(desc->base + index * 0x1000),
                    PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
            }
        }
    }

    DEBUG_OUT("Preparing framebuffer pages for address %p", framebuffer->framebuffer_addr);

    uint64_t fbBase = framebuffer->framebuffer_addr;
    uint64_t fbSize = (uint64_t)framebuffer->framebuffer_height * framebuffer->framebuffer_pitch;

    DEBUG_OUT("Framebuffer width: %u; height: %u; bpp: %u; pitch: %u",
        framebuffer->framebuffer_width, framebuffer->framebuffer_height,
        framebuffer->framebuffer_bpp, framebuffer->framebuffer_pitch);

    for (uint64_t t = fbBase; t < fbBase + fbSize; t += 0x1000)
    {
        pageTableManager.MapMemory((void *)t, (void *)TranslateToPhysicalMemoryAddress((uint64_t)t),
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_WRITE_COMBINE);
    }

    framebuffer->framebuffer_addr = fbBase;

    DEBUG_OUT("%s", "Initialized Framebuffer");

    pageTableManager.MapMemory((void *)TranslateToHighHalfMemoryAddress((uint64_t)globalAllocator.PageBitmap.buffer), (void *)globalAllocator.PageBitmap.buffer,
        PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);

    globalAllocator.PageBitmap.buffer = (uint8_t *)TranslateToHighHalfMemoryAddress((uint64_t)globalAllocator.PageBitmap.buffer);

    Registers::WriteCR3((uint64_t)pageTableManager.p4);

    DEBUG_OUT("Wrote CR3 (%p)", pageTableManager.p4);

    globalPageTableManager = &pageTableManager;

    DEBUG_OUT("%s", "Initialized memory");
}

void InitializeACPI(stivale2_struct_tag_rsdp *rsdp)
{
    printf("Initializing ACPI\n");

    if(rsdp == NULL)
    {
        printf("[ACPI] Missing RSDP!\n");

        return;
    }

    RSDP2 *rsdpStruct = (RSDP2 *)rsdp->rsdp;

    char signature[9] = { 0 };

    char OEMID[7] = { 0 };

    memcpy(signature, rsdpStruct->signature, 8);
    memcpy(OEMID, rsdpStruct->OEMID, 6);

    printf("[ACPI] RSDP Signature: %s\n[ACPI] OEMID: %s\n[ACPI] Revision: %u\n",
        signature,
        OEMID,
        rsdpStruct->revision);

    if(rsdpStruct->revision == 0)
    {
        printf("[ACPI] Unsupported ACPI revision\n");

        return;
    }

    if(rsdpStruct->XSDTAddress == 0)
    {
        printf("[ACPI] Failed to get XSDT!\n");

        return;
    }

    volatile SDTHeader *xsdt = (volatile SDTHeader *)TranslateToHighHalfMemoryAddress(rsdpStruct->XSDTAddress);

    ACPI::DumpTables(xsdt);

    volatile MCFGHeader *mcfg = (volatile MCFGHeader *)ACPI::FindTable(xsdt, "MCFG");

    if(mcfg == NULL)
    {
        printf("[ACPI] Failed to get MCFG!\n");

        return;
    }

    PCI::EnumeratePCI(mcfg);

    FileSystem::globalPartitionManager->Initialize();
}

FramebufferRenderer r = FramebufferRenderer(NULL, NULL);

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

void InitializeKernel(stivale2_struct *stivale2Struct)
{
    InitializeSerial();

    stivale2_struct_tag_framebuffer *framebuffer = (stivale2_struct_tag_framebuffer *)Stivale2GetTag(stivale2Struct, STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID);
    stivale2_struct_tag_memmap *memmap = (stivale2_struct_tag_memmap *)Stivale2GetTag(stivale2Struct, STIVALE2_STRUCT_TAG_MEMMAP_ID);
    stivale2_struct_tag_modules *modules = (stivale2_struct_tag_modules *)Stivale2GetTag(stivale2Struct, STIVALE2_STRUCT_TAG_MODULES_ID);
    stivale2_struct_tag_rsdp *rsdp = (stivale2_struct_tag_rsdp *)Stivale2GetTag(stivale2Struct, STIVALE2_STRUCT_TAG_RSDP_ID);

    stivale2_module *symbols = Stivale2GetModule(modules, "symbols.map");
    stivale2_module *font = Stivale2GetModule(modules, "font.psf");
    stivale2_module *initrd = Stivale2GetModule(modules, "initrd");

    LoadGDT();

    InitializeMemory(memmap, framebuffer);

    printf("Initializing Interrupts\n");

    interrupts.Init();

    if(symbols != NULL)
    {
        DEBUG_OUT("Initializing kernel symbols from size %llu", symbols->end - symbols->begin);

        KernelInitStacktrace((char *)symbols->begin, symbols->end - symbols->begin);
    }

    EnableSSE();

    psf2_font_t *psf2Font = NULL;

    if(font != NULL)
    {
        psf2Font = psf2Load((void *)font->begin);
    }

    Framebuffer *framebufferStruct = (Framebuffer *)malloc(sizeof(Framebuffer));

    framebufferStruct->baseAddress = (void *)framebuffer->framebuffer_addr;
    framebufferStruct->bufferSize = framebuffer->framebuffer_pitch * framebuffer->framebuffer_height;
    framebufferStruct->height = framebuffer->framebuffer_height;
    framebufferStruct->width = framebuffer->framebuffer_width;
    framebufferStruct->pixelsPerScanLine = framebuffer->framebuffer_pitch;

    r = FramebufferRenderer(framebufferStruct, psf2Font);

    globalRenderer = &r;

    globalRenderer->Initialize();

    int consoleWidth = globalRenderer->Width() / globalRenderer->FontWidth();
    int consoleHeight = globalRenderer->Height() / globalRenderer->FontHeight();

    DEBUG_OUT("creating console: width: %d; height: %d;", consoleWidth, consoleHeight);

    console = vtconsole(consoleWidth, consoleHeight, PaintHandler, CursorHandler);

    globalInputSystem.initialize();

    timer.initialize();

    timer->Initialize();

    timer->RegisterHandler(RefreshFramebuffer);

    FileSystem::vfs.initialize();
    FileSystem::globalPartitionManager.initialize();

    InitializeACPI(rsdp);

    printf("Initializing syscalls\n");

    InitializeSyscalls();

    printf("Initializing cmos\n");

    cmos.Initialize();

    if(initrd != NULL)
    {
        FileSystem::tarfs::TarFS *tarfs = new FileSystem::tarfs::TarFS((uint8_t *)initrd->begin);

        FileSystem::vfs->AddMountPoint("/", tarfs);

        //tarfs->DebugListDirectories();
    }

    FileSystem::InitializeVirtualFiles();

    InitializeKeyboard();

    globalProcessManager = new ProcessManager(new RoundRobinScheduler());

    DEBUG_OUT("%s", "Finished initializing the kernel");

    uint64_t MBSize = 1024 * 1024;

    (void)MBSize;

    DEBUG_OUT("Memory Stats: Free: %lluMB; Used: %lluMB; Reserved: %lluMB", globalAllocator.GetFreeRAM() / MBSize,
        globalAllocator.GetUsedRAM() / MBSize, globalAllocator.GetReservedRAM() / MBSize);
}

void _putchar(char character)
{
    if(console != NULL)
    {
        vtconsole_putchar(console, character);
    }
    
    SerialPortOutStreamCOM1(character, NULL);
}

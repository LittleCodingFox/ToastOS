#include "SMP.hpp"
#include "debug.hpp"
#include "printf/printf.h"
#include "paging/PageFrameAllocator.hpp"
#include "paging/PageTableManager.hpp"
#include "registers/Registers.hpp"
#include "interrupts/IDT.hpp"
#include "interrupts/Interrupts.hpp"
#include "sse/sse.hpp"
#include "process/Process.hpp"
#include "printf/printf.h"
#include "lapic/LAPIC.hpp"
#include "Panic.hpp"

static uint8_t stack[SMP_STACK_SIZE];

uint32_t cpuCount = 1;
CPUInfo *cpuInfos = NULL;

uint32_t CPUCount()
{
    return cpuCount;
}

CPUInfo *CurrentCPUInfo()
{
    CPUInfo *info = (CPUInfo *)KernelGSBase();

    for(uint32_t i = 0; i < cpuCount; i++)
    {
        if(info == cpuInfos + i)
        {
            return info;
        }
    }

    DEBUG_OUT("smp: Failed to get current CPU", "");

    return nullptr;
}

void SetKernelGSBase(void *address)
{
    Registers::WriteMSR(0xC0000102, (uint64_t)address);
}

void SetGSBase(void *address)
{
    Registers::WriteMSR(0xC0000101, (uint64_t)address);
}

void *KernelGSBase()
{
    return (void *)Registers::ReadMSR(0xC0000102);
}

void *GSBase()
{
    return (void *)Registers::ReadMSR(0xC0000101);
}

int initializedCPUs = 0;

void BootstrapSMP(stivale2_smp_info *smp)
{
    Registers::WriteCR3((uint64_t)globalPageTableManager->p4);

    EnableSSE();

    CPUInfo *info = &cpuInfos[smp->lapic_id];

    info->gdt = (GDT *)TranslateToHighHalfMemoryAddress((uint64_t)globalAllocator.RequestPage());

    *info->gdt = {
        {
            .limitLow = 0,
            .baseLow = 0,
            .baseMiddle = 0,
            .accessFlag = 0x00,
            .limitFlags = 0x00,
            .baseHigh = 0
        }, // null
        {
            .limitLow = 0,
            .baseLow = 0,
            .baseMiddle = 0,
            .accessFlag = GDTAccessKernelCode,
            .limitFlags = 0xA0, 
            .baseHigh = 0
        }, // kernel code segment
        {
            .limitLow = 0,
            .baseLow = 0,
            .baseMiddle = 0,
            .accessFlag = GDTAccessKernelData,
            .limitFlags = 0x80,
            .baseHigh = 0
        }, // kernel data segment
        {
            .limitLow = 0,
            .baseLow = 0,
            .baseMiddle = 0,
            .accessFlag = 0x00,
            .limitFlags = 0x00,
            .baseHigh = 0
        }, // user null
        {
            .limitLow = 0,
            .baseLow = 0,
            .baseMiddle = 0,
            .accessFlag = GDTAccessUserData,
            .limitFlags = 0x80,
            .baseHigh = 0
        }, // user data segment
        {
            .limitLow = 0,
            .baseLow = 0,
            .baseMiddle = 0,
            .accessFlag = GDTAccessUserCode,
            .limitFlags = 0xA0,
            .baseHigh = 0
        }, // user code segment
        {
            .length = 104,
            .flags = 0b10001001,
        }, //TSS
    };

    info->gdtr = {
        .size = sizeof(GDT) - 1,
        .offset = (uint64_t)info->gdt
    };

    info->tss = {0};

    idt.Load();

    LoadGDT(info->gdt, &info->tss, info->tssStack, info->ist2Stack, sizeof(info->tssStack), &info->gdtr);

    SetKernelGSBase(&cpuInfos[smp->lapic_id]);

    //Enable write protection
    Registers::WriteCR0(Registers::ReadCR0() | (1 << 16));

    // Program the PAT. Each byte configures a single entry.
    // 00: Uncacheable
    // 01: Write Combining
    // 04: Write Through
    // 06: Write Back
    //Registers::WriteMSR(0x277, 0x00'00'01'00'00'00'04'06);
    Registers::WriteMSR(0x0277, 0x0000000005010406);

    InitializeLAPIC();

    char buffer[100];

    sprintf(buffer, "KernelTask %i", smp->lapic_id);

    processManager->CreateFromEntryPoint((uint64_t)KernelTask, buffer, "/home/toast/", PROCESS_PERMISSION_KERNEL);

    initializedCPUs++;

    interrupts.EnableInterrupts();

    processManager->Wait();
}

void InitializeSMP(stivale2_struct_tag_smp *smp)
{
    cpuCount = smp->cpu_count;

    DEBUG_OUT("smp: Initializing %i CPUs", cpuCount);

    cpuInfos = new CPUInfo[cpuCount];

    for(uint64_t i = 0; i < smp->cpu_count; i++)
    {
        struct stivale2_smp_info *smpInfo = &smp->smp_info[i];

        cpuInfos[i].APICID = smpInfo->lapic_id;

        if(smpInfo->lapic_id == smp->bsp_lapic_id)
        {
            cpuInfos[i].stack = stack;
            cpuInfos[i].bsp = true;
            cpuInfos[i].APICID = smp->bsp_lapic_id;

            initializedCPUs++;

            SetKernelGSBase(&cpuInfos[smpInfo->lapic_id]);

            continue;
        }

        cpuInfos[i].stack = new uint8_t[SMP_STACK_SIZE];
        cpuInfos[i].bsp = false;

        for(uint32_t j = 0; j < SMP_STACK_SIZE; j += 0x1000)
        {
            globalPageTableManager->MapMemory((void *)&cpuInfos[i].stack[j],
                (void *)TranslateToPhysicalMemoryAddress((uint64_t)&cpuInfos[i].stack[j]),
                PAGING_FLAG_WRITABLE);
        }

        smpInfo->target_stack = (uint64_t)cpuInfos[i].stack + SMP_STACK_SIZE;

        smpInfo->goto_address = (uint64_t)BootstrapSMP;
    }

    while(initializedCPUs != cpuCount)
    {
        asm volatile("hlt");
    }
}

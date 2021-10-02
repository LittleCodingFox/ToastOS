#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "Process.hpp"
#include "debug.hpp"
#include "gdt/gdt.hpp"
#include "paging/PageFrameAllocator.hpp"
#include "paging/PageTableManager.hpp"
#include "ports/Ports.hpp"
#include "registers/Registers.hpp"
#include "timer/Timer.hpp"
#include "syscall/syscall.hpp"
#include "stacktrace/stacktrace.hpp"

ProcessManager *globalProcessManager;
uint64_t processIDCounter = 0;

extern "C" void SwitchToUsermode(void *instructionPointer, void *stackPointer);
extern "C" void SwitchTasks(ProcessControlBlock* next);

void SwitchProcess(InterruptStack *stack)
{
    //DEBUG_OUT("SWITCH PROCESS %p", stack);

    globalProcessManager->SwitchProcess(stack, true);
}

ProcessManager::ProcessManager(IScheduler *scheduler) : scheduler(scheduler)
{
    timer.RegisterHandler(::SwitchProcess);
}

void ProcessManager::SwitchProcess(InterruptStack *stack, bool fromTimer)
{
    interrupts.DisableInterrupts(); //Will be enabled in the switch task call

    lock.Lock();

    ProcessControlBlock *current = scheduler->CurrentProcess();
    ProcessControlBlock *next = scheduler->NextProcess();

    if(current == NULL || next == NULL)
    {
        lock.Unlock();

        if(fromTimer)
        {
            //Called from timer, so must finish up PIC
            outport8(PIC1, PIC_EOI);
        }

        return;
    }

    //DEBUG_OUT("Switching processes %p to %p", current, next);

    if(current == next)
    {
        lock.Unlock();

        if(fromTimer)
        {
            //Called from timer, so must finish up PIC
            outport8(PIC1, PIC_EOI);
        }

        return;
    }

    if(current->state == PROCESS_STATE_RUNNING)
    {
        current->r10 = stack->r10;
        current->r11 = stack->r11;
        current->r12 = stack->r12;
        current->r13 = stack->r13;
        current->r14 = stack->r14;
        current->r15 = stack->r15;
        current->r8 = stack->r8;
        current->r9 = stack->r9;
        current->rax = stack->rax;
        current->rbp = stack->rbp;
        current->rbx = stack->rbx;
        current->rcx = stack->rcx;
        current->rdi = stack->rdi;
        current->rdx = stack->rdx;
        current->rsi = stack->rsi;
        current->rsp = stack->stackPointer;
        current->rip = stack->instructionPointer;
        current->rflags = stack->cpuFlags;

        if(current->process->permissionLevel == PROCESS_PERMISSION_KERNEL)
        {
            current->cs = (GDTKernelBaseSelector + 0x00);
            current->ss = (GDTKernelBaseSelector + 0x08);
        }
        else
        {
            current->cs = (GDTUserBaseSelector + 0x10) | 3;
            current->ss = (GDTUserBaseSelector + 0x08) | 3;
        }
    }

    if(next->state == PROCESS_STATE_NEEDS_INIT)
    {
        next->state = PROCESS_STATE_RUNNING;

        //DEBUG_OUT("Initializing task %p: rsp: %p; rip: %p; cr3: %p", next, next->rsp, next->rip, next->cr3);

        if(fromTimer)
        {
            //Called from timer, so must finish up PIC
            outport8(PIC1, PIC_EOI);
        }

        lock.Unlock();

        SwitchTasks(next);

        return;
    }

    lock.Unlock();

    /*
    DEBUG_OUT("Switching tasks:\n\trsp: %p\n\trip: %p\n\tcr3: %p\n\tcs: 0x%x\n\tss: 0x%x\nnext:\n\trsp: %p\n\trip: %p\n\tcr3: %p\n\tcs: 0x%x\n\tss: 0x%x",
        current->rsp, current->rip, current->cr3, current->cs, current->ss,
        next->rsp, next->rip, next->cr3, next->cs, next->ss);
    */

    if(fromTimer)
    {
        //Called from timer, so must finish up PIC
        outport8(PIC1, PIC_EOI);
    }

    SwitchTasks(next);
}

ProcessInfo *ProcessManager::CurrentProcess()
{
    lock.Lock();

    ProcessInfo *current = scheduler->CurrentProcess()->process;

    lock.Unlock();

    return current;
}

ProcessInfo *ProcessManager::CreateFromEntryPoint(uint64_t entryPoint, const char *name, uint64_t permissionLevel)
{
    lock.Lock();

    PageTable *pageTableFrame = (PageTable *)globalAllocator.RequestPage();

    globalPageTableManager->MapMemory((void *)TranslateToHighHalfMemoryAddress((uint64_t)pageTableFrame),
        pageTableFrame, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);

    PageTable *higherPageTableFrame = (PageTable *)TranslateToHighHalfMemoryAddress((uint64_t)pageTableFrame);

    memset(higherPageTableFrame, 0, sizeof(PageTable));
    
    for(uint64_t i = 256; i < 512; i++)
    {
        higherPageTableFrame->entries[i] = globalPageTableManager->p4->entries[i];
    }

    PageTableManager userPageManager;
    userPageManager.p4 = pageTableFrame;

    ProcessInfo *newProcess = (ProcessInfo *)globalAllocator.RequestPages(sizeof(ProcessInfo) / 0x1000 + 1);

    uint64_t page = (uint64_t)newProcess / 0x1000;
    uint64_t offset = (uint64_t)newProcess % 0x1000;

    uint64_t pageCount = (sizeof(ProcessInfo) + offset) / 0x1000 + 1;

    for(uint64_t i = 0; i < pageCount; i++)
    {
        globalPageTableManager->MapMemory((void *)((uint64_t)TranslateToHighHalfMemoryAddress((page + i) * 0x1000)),
            (void *)((uint64_t)(page + i) * 0x1000),
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
    }

    newProcess = (ProcessInfo *)TranslateToHighHalfMemoryAddress((uint64_t)newProcess);

    page = (uint64_t)newProcess->stack / 0x1000;
    offset = (uint64_t)newProcess->stack % 0x1000;
    pageCount = (sizeof(newProcess->stack) + offset) / 0x1000 + 1;

    for(uint64_t i = 0; i < pageCount; i++)
    {
        globalPageTableManager->MapMemory((void *)(uint64_t)((page + i) * 0x1000),
            (void *)((uint64_t)TranslateToPhysicalMemoryAddress((page + i) * 0x1000)),
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE);

        //Do the same for the user
        userPageManager.MapMemory((void *)(uint64_t)((page + i) * 0x1000),
            (void *)((uint64_t)TranslateToPhysicalMemoryAddress((page + i) * 0x1000)),
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE);
    }

    memset(newProcess, 0, sizeof(ProcessInfo));
    newProcess->ID = ++processIDCounter;
    newProcess->permissionLevel = permissionLevel;

    newProcess->name = strdup(name);

    //TODO: proper env

    char **env = (char **)calloc(1, sizeof(char *[10]));

    newProcess->environment = env;

    void *stack = (void *)&newProcess->stack[PROCESS_STACK_SIZE];

    PushToStack(stack, entryPoint);

    /*
    PushToStack(stack, env);
    PushToStack(stack, _argv);
    PushToStack(stack, argc);
    */

    newProcess->rsp = (uint64_t)stack;
    newProcess->rip = entryPoint;
    newProcess->rflags = 0x202;

    newProcess->cr3 = (uint64_t)pageTableFrame;

    scheduler->AddProcess(newProcess);

    lock.Unlock();

    return newProcess;
}

ProcessInfo *ProcessManager::LoadImage(const void *image, const char *name, const char **argv, uint64_t permissionLevel)
{
    lock.Lock();

    PageTable *pageTableFrame = (PageTable *)globalAllocator.RequestPage();

    globalPageTableManager->MapMemory((void *)TranslateToHighHalfMemoryAddress((uint64_t)pageTableFrame), pageTableFrame,
        PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);

    PageTable *higherPageTableFrame = (PageTable *)TranslateToHighHalfMemoryAddress((uint64_t)pageTableFrame);

    memset(higherPageTableFrame, 0, sizeof(PageTable));
    
    for(uint64_t i = 256; i < 512; i++)
    {
        higherPageTableFrame->entries[i] = globalPageTableManager->p4->entries[i];
    }

    PageTableManager userPageManager;
    userPageManager.p4 = pageTableFrame;

    ProcessInfo *newProcess = (ProcessInfo *)globalAllocator.RequestPages(sizeof(ProcessInfo) / 0x1000 + 1);

    uint64_t page = (uint64_t)newProcess / 0x1000;
    uint64_t offset = (uint64_t)newProcess % 0x1000;

    uint64_t pageCount = (sizeof(ProcessInfo) + offset) / 0x1000 + 1;

    for(uint64_t i = 0; i < pageCount; i++)
    {
        globalPageTableManager->MapMemory((void *)((uint64_t)TranslateToHighHalfMemoryAddress((page + i) * 0x1000)),
            (void *)((uint64_t)(page + i) * 0x1000),
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
    }

    newProcess = (ProcessInfo *)TranslateToHighHalfMemoryAddress((uint64_t)newProcess);

    page = (uint64_t)newProcess->stack / 0x1000;
    offset = (uint64_t)newProcess->stack % 0x1000;
    pageCount = (sizeof(newProcess->stack) + offset) / 0x1000 + 1;

    for(uint64_t i = 0; i < pageCount; i++)
    {
        globalPageTableManager->MapMemory((void *)(uint64_t)((page + i) * 0x1000),
            (void *)((uint64_t)TranslateToPhysicalMemoryAddress((page + i) * 0x1000)),
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE);

        //Do the same for the user
        userPageManager.MapMemory((void *)(uint64_t)((page + i) * 0x1000),
            (void *)((uint64_t)TranslateToPhysicalMemoryAddress((page + i) * 0x1000)),
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE);
    }

    memset(newProcess, 0, sizeof(ProcessInfo));
    newProcess->ID = ++processIDCounter;
    newProcess->permissionLevel = permissionLevel;

    newProcess->name = strdup(name);

    int argc = 0;

    while(argv[argc])
    {
        argc++;
    }

    char **_argv = (char **)malloc(sizeof(char *[argc + 1]));

    for(uint32_t i = 0; i < argc; i++)
    {
        _argv[i] = strdup(argv[i]);
    }

    _argv[argc] = NULL;

    newProcess->argv = _argv;

    //TODO: proper env

    char **env = (char **)calloc(1, sizeof(char *[10]));

    newProcess->environment = env;

    void *stack = (void *)&newProcess->stack[PROCESS_STACK_SIZE];

    /*
    PushToStack(stack, env);
    PushToStack(stack, _argv);
    PushToStack(stack, argc);
    */

    newProcess->rsp = (uint64_t)stack;
    newProcess->rflags = 0x202;

    newProcess->cr3 = /*Registers::ReadCR3(); //*/(uint64_t)pageTableFrame;

    Elf::ElfHeader *elf = Elf::LoadElf(image);

    newProcess->elf = elf;

    if(elf != NULL)
    {
        Elf::MapElfSegments(elf, /*globalPageTableManager*/&userPageManager);

        newProcess->rip = elf->entry;
    }

    scheduler->AddProcess(newProcess);

    lock.Unlock();

    return newProcess;
}

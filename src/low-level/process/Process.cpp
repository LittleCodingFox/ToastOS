#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "Process.hpp"
#include "debug.hpp"
#include "paging/PageFrameAllocator.hpp"
#include "paging/PageTableManager.hpp"
#include "ports/Ports.hpp"
#include "registers/Registers.hpp"
#include "timer/Timer.hpp"

ProcessManager *globalProcessManager;
uint64_t processIDCounter = 0;

extern "C" void SwitchToUsermode(void *instructionPointer, void *stackPointer);
extern "C" void SwitchTasks(ProcessControlBlock* next);

void SwitchProcess(InterruptStack *stack)
{
    DEBUG_OUT("SWITCH PROCESS %p", stack);

    globalProcessManager->SwitchProcess(stack);
}

ProcessManager::ProcessManager(IScheduler *scheduler) : scheduler(scheduler)
{
    timer.RegisterHandler(::SwitchProcess);
}

void ProcessManager::SwitchProcess(InterruptStack *stack)
{
    interrupts.DisableInterrupts(); //Will be enabled in the switch task call

    lock.Lock();

    ProcessControlBlock *current = scheduler->CurrentProcess();
    ProcessControlBlock *next = scheduler->NextProcess();

    if(current == NULL || next == NULL)
    {
        lock.Unlock();

        //Called from timer, so must finish up PIC
        outport8(PIC1, PIC_EOI);

        return;
    }

    DEBUG_OUT("Switching processes %p to %p", current, next);

    if(current->state == PROCESS_STATE_NEEDS_INIT)
    {
        current->state = PROCESS_STATE_RUNNING;

        lock.Unlock();

        DEBUG_OUT("Initializing task: rsp: %p; rip: %p; cr3: %p", current->rsp, current->rip, current->cr3);

        //Called from timer, so must finish up PIC
        outport8(PIC1, PIC_EOI);

        SwitchTasks(current);

        DEBUG_OUT("AFTER %i", 0);

        return;
    }

    if(current == next)
    {
        lock.Unlock();

        //Called from timer, so must finish up PIC
        outport8(PIC1, PIC_EOI);

        return;
    }

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

    if(next->state == PROCESS_STATE_NEEDS_INIT)
    {
        next->state = PROCESS_STATE_RUNNING;

        lock.Unlock();

        DEBUG_OUT("Initializing task: rsp: %p; rip: %p; cr3: %p", next->rsp, next->rip, next->cr3);

        //Called from timer, so must finish up PIC
        outport8(PIC1, PIC_EOI);

        SwitchTasks(next);

        DEBUG_OUT("AFTER %i", 1);

        return;
    }

    lock.Unlock();

    DEBUG_OUT("Switching tasks: rsp: %p; rip: %p; cr3: %p -> rsp: %p; rip: %p; cr3: %p", current->rsp, current->rip, current->cr3, next->rsp, next->rip, next->cr3);

    //Called from timer, so must finish up PIC
    outport8(PIC1, PIC_EOI);

    SwitchTasks(next);
}

ProcessInfo *ProcessManager::CurrentProcess()
{
    return scheduler->CurrentProcess()->process;
}

ProcessInfo *ProcessManager::CreateFromEntryPoint(uint64_t entryPoint, const char *name)
{
    Threading::ScopedLock lock(this->lock);

    ProcessInfo *newProcess = (ProcessInfo *)globalAllocator.RequestPages(sizeof(ProcessInfo) / 0x1000 + 1);

    uint64_t page = (uint64_t)newProcess / 0x1000;

    uint64_t pageCount = sizeof(ProcessInfo) / 0x1000 + 1;

    for(uint64_t i = 0; i < pageCount; i++)
    {
        globalPageTableManager->IdentityMap((void *)((uint64_t)(page + i) * 0x1000), PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
    }

    page = (uint64_t)newProcess->stack / 0x1000;
    pageCount = sizeof(newProcess->stack) / 0x1000 + 1;

    for(uint64_t i = 0; i < pageCount; i++)
    {
        globalPageTableManager->IdentityMap((void *)((uint64_t)(page + i) * 0x1000), PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE);
    }

    memset(newProcess, 0, sizeof(ProcessInfo));
    newProcess->ID = ++processIDCounter;

    newProcess->name = strdup(name);

    memset(newProcess->stack, 0, sizeof(uint64_t[PROCESS_STACK_SIZE]));

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

    PageTable *pageTableFrame = (PageTable *)globalAllocator.RequestPage();

    globalPageTableManager->IdentityMap(pageTableFrame, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);

    memset(pageTableFrame, 0, sizeof(PageTable));
    
    for(uint64_t i = 256; i < 512; i++)
    {
        pageTableFrame->entries[i] = globalPageTableManager->p4->entries[i];
    }

    newProcess->cr3 = Registers::ReadCR3(); //(uint64_t)pageTableFrame;

    scheduler->AddProcess(newProcess);

    return newProcess;
}

ProcessInfo *ProcessManager::LoadImage(const void *image, const char *name, const char **argv)
{
    Threading::ScopedLock lock(this->lock);

    ProcessInfo *newProcess = (ProcessInfo *)globalAllocator.RequestPages(sizeof(ProcessInfo) / 0x1000 + 1);

    uint64_t page = (uint64_t)newProcess / 0x1000;

    uint64_t pageCount = sizeof(ProcessInfo) / 0x1000 + 1;

    for(uint64_t i = 0; i < pageCount; i++)
    {
        globalPageTableManager->IdentityMap((void *)((uint64_t)(page + i) * 0x1000), PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
    }

    page = (uint64_t)newProcess->stack / 0x1000;
    pageCount = sizeof(newProcess->stack) / 0x1000 + 1;

    for(uint64_t i = 0; i < pageCount; i++)
    {
        globalPageTableManager->IdentityMap((void *)((uint64_t)(page + i) * 0x1000), PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE);
    }

    memset(newProcess, 0, sizeof(ProcessInfo));
    newProcess->ID = ++processIDCounter;
        /*
    }
    else
    {
        currentProcess->ID++;

        previous = currentProcess->elf;

        free(currentProcess->name);

        for(uint32_t i = 0; currentProcess->argv[i] != NULL; i++)
        {
            free(currentProcess->argv[i]);
        }

        free(currentProcess->argv);

        for(uint32_t i = 0; currentProcess->environment[i] != NULL; i++)
        {
            free(currentProcess->environment[i]);
        }

        free(currentProcess->environment);
    }
    */

    newProcess->name = strdup(name);

    memset(newProcess->stack, 0, sizeof(uint64_t[PROCESS_STACK_SIZE]));

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

    PageTable *pageTableFrame = (PageTable *)globalAllocator.RequestPage();

    globalPageTableManager->IdentityMap(pageTableFrame, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);

    memset(pageTableFrame, 0, sizeof(PageTable));
    
    for(uint64_t i = 256; i < 512; i++)
    {
        pageTableFrame->entries[i] = globalPageTableManager->p4->entries[i];
    }

    newProcess->cr3 = Registers::ReadCR3(); //(uint64_t)pageTableFrame;

    Elf::ElfHeader *elf = Elf::LoadElf(image);

    newProcess->elf = elf;

    if(elf != NULL)
    {
        Elf::MapElfSegments(elf, globalPageTableManager);

        newProcess->rip = elf->entry;
    }

    scheduler->AddProcess(newProcess);

    return newProcess;
}

void ProcessManager::ExecuteProcess(ProcessInfo *process)
{
    Threading::ScopedLock lock(this->lock);

    if(process == NULL || process->elf == NULL)
    {
        DEBUG_OUT("Failed to execute process %p: Invalid pointer or missing elf image", process);

        return;
    }

    DEBUG_OUT("Executing process %p", process);

    uint64_t ip = process->elf->entry;
    uint64_t stackPointer = process->rsp;
    uint64_t cr3 = process->cr3;

    //Registers::WriteCR3(cr3);

    SwitchToUsermode((void *)ip, (void *)stackPointer);
}

void ProcessManager::SwitchToUsermode(void *instructionPointer, void *stackPointer)
{
    Threading::ScopedLock lock(this->lock);

    DEBUG_OUT("Switching to usermode at instruction %p and stack %p", instructionPointer, stackPointer);

    ::SwitchToUsermode(instructionPointer, stackPointer);
}

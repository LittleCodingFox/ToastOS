#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "Process.hpp"
#include "debug.hpp"
#include "fcntl.h"
#include "gdt/gdt.hpp"
#include "paging/PageFrameAllocator.hpp"
#include "paging/PageTableManager.hpp"
#include "ports/Ports.hpp"
#include "registers/Registers.hpp"
#include "timer/Timer.hpp"
#include "syscall/syscall.hpp"
#include "stacktrace/stacktrace.hpp"
#include "filesystems/VFS.hpp"

using namespace FileSystem;

#define LD_BASE 0x40000000
#define push(stack, value) *(--stack) = (value)

ProcessManager *globalProcessManager;
uint64_t processIDCounter = 0;

extern "C" void SwitchToUsermode(void *instructionPointer, void *stackPointer);
extern "C" void SwitchTasks(ProcessControlBlock* next);

void SwitchProcess(InterruptStack *stack)
{
    if(globalProcessManager->IsLocked())
    {
        return;
    }

    //DEBUG_OUT("SWITCH PROCESS %p", stack);

    globalProcessManager->SwitchProcess(stack, true);
}

ProcessManager::ProcessManager(IScheduler *scheduler) : scheduler(scheduler)
{
    if(timer)
    {
        timer->RegisterHandler(::SwitchProcess);
    }
}

bool ProcessManager::IsLocked()
{
    return lock.IsLocked();
}

void ProcessManager::SwitchProcess(InterruptStack *stack, bool fromTimer)
{
    lock.Lock();

    interrupts.DisableInterrupts(); //Will be enabled in the switch task call

    ProcessControlBlock *current = scheduler->CurrentProcess();
    ProcessControlBlock *next = scheduler->NextProcess();

    if(current == NULL || next == NULL)
    {
        if(fromTimer)
        {
            //Called from timer, so must finish up PIC
            outport8(PIC1, PIC_EOI);
        }

        lock.Unlock();

        return;
    }

    //DEBUG_OUT("Switching processes %p to %p", current, next);

    if(current == next)
    {
        if(fromTimer)
        {
            //Called from timer, so must finish up PIC
            outport8(PIC1, PIC_EOI);
        }

        lock.Unlock();

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
        current->fsBase = current->process->fsBase;

        asm volatile(" fxsave %0" :: "m"(current->FXSAVE));

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

    asm volatile(" fxrstor %0" : : "m"(next->FXSAVE));

    LoadFSBase(next->fsBase);

    lock.Unlock();

    SwitchTasks(next);
}

ProcessInfo *ProcessManager::CurrentProcess()
{
    lock.Lock();

    ProcessInfo *current = NULL;

    if(scheduler != NULL && scheduler->CurrentProcess() != NULL)
    {
        current = scheduler->CurrentProcess()->process;
    }

    lock.Unlock();

    return current;
}

ProcessInfo *ProcessManager::CreateFromEntryPoint(uint64_t entryPoint, const char *name, const char *cwd, uint64_t permissionLevel)
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
    newProcess->cwd = cwd;
    newProcess->state = PROCESS_STATE_NEEDS_INIT;

    newProcess->name = strdup(name);

    for(uint64_t i = 0; i < SIGNAL_MAX; i++)
    {
        newProcess->sigHandlers[i].sa_handler = SIG_DFL;
    }

    //TODO: proper env

    char **env = (char **)calloc(1, sizeof(char *[10]));

    newProcess->environment = env;

    uint64_t *stack = (uint64_t *)&newProcess->stack[PROCESS_STACK_SIZE];

    push(stack, entryPoint);

    newProcess->rsp = (uint64_t)stack;
    newProcess->rip = entryPoint;
    newProcess->rflags = 0x202;

    newProcess->cr3 = (uint64_t)pageTableFrame;

    scheduler->AddProcess(newProcess);

    processes.push_back(newProcess);

    DEBUG_OUT("Initializing entry point process at RIP %p; RSP: %p; CR3: %p", newProcess->rip, newProcess->rsp, newProcess->cr3);

    lock.Unlock();

    return newProcess;
}

ProcessInfo *ProcessManager::LoadImage(const void *image, const char *name, const char **argv, const char **envp, const char *cwd, uint64_t permissionLevel)
{
    lock.Lock();

    PageTable *pageTableFrame = (PageTable *)globalAllocator.RequestPage();
    PageTable *currentTable = (PageTable *)Registers::ReadCR3();
    PageTableManager currentPageManager;
    currentPageManager.p4 = currentTable;

    currentPageManager.MapMemory((void *)TranslateToHighHalfMemoryAddress((uint64_t)pageTableFrame), pageTableFrame,
        PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);

    PageTable *higherPageTableFrame = (PageTable *)TranslateToHighHalfMemoryAddress((uint64_t)pageTableFrame);

    memset(higherPageTableFrame, 0, sizeof(PageTable));
    
    for(uint64_t i = 256; i < 512; i++)
    {
        higherPageTableFrame->entries[i] = currentPageManager.p4->entries[i];
    }

    PageTableManager userPageManager;
    userPageManager.p4 = pageTableFrame;

    ProcessInfo *newProcess = (ProcessInfo *)globalAllocator.RequestPages(sizeof(ProcessInfo) / 0x1000 + 1);

    uint64_t page = (uint64_t)newProcess / 0x1000;
    uint64_t offset = (uint64_t)newProcess % 0x1000;

    uint64_t pageCount = (sizeof(ProcessInfo) + offset) / 0x1000 + 1;

    for(uint64_t i = 0; i < pageCount; i++)
    {
        currentPageManager.MapMemory((void *)((uint64_t)TranslateToHighHalfMemoryAddress((page + i) * 0x1000)),
            (void *)((uint64_t)(page + i) * 0x1000),
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
    }

    newProcess = (ProcessInfo *)TranslateToHighHalfMemoryAddress((uint64_t)newProcess);

    page = (uint64_t)newProcess->stack / 0x1000;
    offset = (uint64_t)newProcess->stack % 0x1000;
    pageCount = (sizeof(newProcess->stack) + offset) / 0x1000 + 1;

    for(uint64_t i = 0; i < pageCount; i++)
    {
        currentPageManager.MapMemory((void *)(uint64_t)((page + i) * 0x1000),
            (void *)((uint64_t)TranslateToPhysicalMemoryAddress((page + i) * 0x1000)),
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE);

        //Map as identity for the process page table
        userPageManager.IdentityMap((void *)(uint64_t)TranslateToPhysicalMemoryAddress((page + i) * 0x1000),
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE);
    }

    userPageManager.MapMemory((void *)TranslateToHighHalfMemoryAddress((uint64_t)pageTableFrame), pageTableFrame,
        PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE);

    memset(newProcess, 0, sizeof(ProcessInfo));

    newProcess->ID = ++processIDCounter;
    newProcess->permissionLevel = permissionLevel;
    newProcess->cwd = cwd;
    newProcess->state = PROCESS_STATE_NEEDS_INIT;

    newProcess->name = strdup(name);

    newProcess->rflags = 0x202;

    newProcess->cr3 = (uint64_t)pageTableFrame;

    for(uint64_t i = 0; i < SIGNAL_MAX; i++)
    {
        newProcess->sigHandlers[i].sa_handler = SIG_DFL;
    }

    Elf::Auxval auxval;
    char *ldPath = NULL;

    Elf::ElfHeader *elf = Elf::LoadElf(image, 0, &auxval);

    newProcess->elf = elf;

    uint64_t rip;

    if(elf != NULL)
    {
        Elf::MapElfSegments(elf, &userPageManager, 0, &auxval, &ldPath);

        if(ldPath != NULL)
        {
            DEBUG_OUT("Found LD for process: %s", ldPath);

            FILE_HANDLE ldHandle = vfs->OpenFile(ldPath, O_RDONLY, newProcess);

            if(vfs->FileType(ldHandle) != FILE_HANDLE_FILE)
            {
                DEBUG_OUT("Failed to load ld binary at %s", ldPath);

                lock.Unlock();

                //TODO: Cleanup

                return NULL;
            }
            else
            {
                uint64_t fileSize = vfs->FileLength(ldHandle);

                DEBUG_OUT("Loading LD with size %llu", fileSize);

                uint8_t *ldImage = new uint8_t[fileSize];

                if(vfs->ReadFile(ldHandle, ldImage, fileSize) != fileSize)
                {
                    DEBUG_OUT("Failed to load ld binary at %s: I/O Error", ldPath);

                    lock.Unlock();

                    //TODO: Cleanup
                    return NULL;
                }

                Elf::Auxval ldAuxval;

                Elf::ElfHeader *ldElf = Elf::LoadElf(ldImage, LD_BASE, &ldAuxval);

                if(ldElf == NULL)
                {
                    DEBUG_OUT("Invalid ld binary at %s", ldPath);

                    lock.Unlock();

                    //TODO: Cleanup
                    return NULL;
                }
                else
                {
                    Elf::MapElfSegments(ldElf, &userPageManager, LD_BASE, &ldAuxval, &ldPath);

                    rip = ldAuxval.entry;
                }
            }
        }
        else
        {
            rip = auxval.entry;
        }
    }
    else
    {
        lock.Unlock();

        //TODO: Cleanup
        return NULL;
    }

    uint64_t stackEnd = (uint64_t)&newProcess->stack[PROCESS_STACK_SIZE];
    uint64_t *stack = (uint64_t *)stackEnd;
    uint8_t *byteStack = (uint8_t *)stack;

    uint32_t envc = 0;

    while(envp[envc])
    {
        uint32_t length = strlen(envp[envc]) + 1;
        byteStack = (uint8_t *)byteStack - length;

        memcpy(byteStack, envp[envc], length);

        envc++;
    }

    uint32_t argc = 0;

    while(argv[argc])
    {
        uint32_t length = strlen(argv[argc]) + 1;
        byteStack = (uint8_t *)byteStack - length;
        memcpy(byteStack, argv[argc], length);

        argc++;
    }

    byteStack -= (size_t)byteStack & 0xF;

    stack = (uint64_t *)byteStack;

    if(((argc + envc + 1) & 1) != 0)
    {
        stack--;
    }

    //Zero aux vector entry
    push(stack, 0);
    push(stack, 0);

    //Entry
    push(stack, auxval.entry);
    push(stack, AT_ENTRY);

    //PHDR
    push(stack, auxval.phdr);
    push(stack, AT_PHDR);

    //PHENT
    push(stack, auxval.programHeaderSize);
    push(stack, AT_PHENT);

    //PHNUM
    push(stack, auxval.phNum);
    push(stack, AT_PHNUM);

    offset = 0;

    push(stack, 0);

    stack -= envc;

    for(uint32_t i = 0; i < envc; i++)
    {
        stack[i] = stackEnd - (offset += strlen(envp[i]) + 1);
    }

    push(stack, 0);

    stack -= argc;

    for(uint32_t i = 0; i < argc; i++)
    {
        stack[i] = stackEnd - (offset += strlen(argv[i]) + 1);
    }

    push(stack, argc);

    newProcess->rsp = TranslateToPhysicalMemoryAddress((uint64_t)stack);
    newProcess->rip = rip;

    DEBUG_OUT("Initializing process at RIP %p auxval entry: %p RSP: %p; CR3: %p", rip, auxval.entry, newProcess->rsp, newProcess->cr3);

    scheduler->AddProcess(newProcess);

    processes.push_back(newProcess);

    lock.Unlock();

    return newProcess;
}

void ProcessManager::LoadFSBase(uint64_t base)
{
    Registers::WriteMSR(0xC0000100, base);
}

void ProcessManager::Exit(int exitCode)
{
    lock.Lock();

    if(scheduler != NULL && scheduler->CurrentProcess() != NULL)
    {
        auto pcb = scheduler->CurrentProcess();

        pcb->state = pcb->process->state = PROCESS_STATE_DEAD;
        pcb->process->exitCode = exitCode;

        scheduler->ExitProcess(pcb->process);
    }

    lock.Unlock();

    auto pcb = scheduler->CurrentProcess();

    SwitchTasks(pcb);
}

void ProcessManager::Sigaction(int signum, sigaction *act, sigaction *oldact)
{
    if(signum < 0 || signum >= SIGNAL_MAX)
    {
        return;
    }

    ProcessInfo *currentProcess = CurrentProcess();

    lock.Lock();

    if(act)
    {
        currentProcess->sigHandlers[signum] = *act;
    }

    if(oldact)
    {
        *oldact = currentProcess->sigHandlers[signum];
    }

    lock.Unlock();
}

void ProcessManager::SetUID(pid_t pid, uid_t uid)
{
    ProcessInfo *process = GetProcess(pid);

    Threading::ScopedLock Lock(lock);

    if(process == NULL)
    {
        return;
    }

    process->uid = uid;
}

uid_t ProcessManager::GetUID(pid_t pid)
{
    ProcessInfo *process = GetProcess(pid);

    Threading::ScopedLock Lock(lock);

    if(process == NULL)
    {
        return 0;
    }

    return process->uid;
}

void ProcessManager::SetGID(pid_t pid, gid_t gid)
{
    ProcessInfo *process = GetProcess(pid);

    Threading::ScopedLock Lock(lock);

    if(process == NULL)
    {
        return;
    }

    process->gid = gid;
}

uid_t ProcessManager::GetGID(pid_t pid)
{
    ProcessInfo *process = GetProcess(pid);

    Threading::ScopedLock Lock(lock);

    if(process == NULL)
    {
        return 0;
    }

    return process->gid;
}

int32_t ProcessManager::Fork(InterruptStack *interruptStack, pid_t *child)
{
    lock.Lock();

    auto current = scheduler->CurrentProcess();

    PageTable *pageTableFrame = (PageTable *)globalAllocator.RequestPage();
    PageTable *currentTable = (PageTable *)Registers::ReadCR3();
    PageTable *higherCurrentTable = (PageTable *)TranslateToHighHalfMemoryAddress((uint64_t)currentTable);
    PageTableManager currentPageManager;
    currentPageManager.p4 = currentTable;

    currentPageManager.MapMemory((void *)TranslateToHighHalfMemoryAddress((uint64_t)pageTableFrame), pageTableFrame,
        PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE);

    PageTable *higherPageTableFrame = (PageTable *)TranslateToHighHalfMemoryAddress((uint64_t)pageTableFrame);

    memset(higherPageTableFrame, 0, sizeof(PageTable));

    for(uint64_t i = 0; i < 512; i++)
    {
        higherPageTableFrame->entries[i] = higherCurrentTable->entries[i];
    }

    PageTableManager userPageManager;
    userPageManager.p4 = pageTableFrame;

    ProcessInfo *newProcess = (ProcessInfo *)globalAllocator.RequestPages(sizeof(ProcessInfo) / 0x1000 + 1);

    uint64_t page = (uint64_t)newProcess / 0x1000;
    uint64_t offset = (uint64_t)newProcess % 0x1000;

    uint64_t pageCount = (sizeof(ProcessInfo) + offset) / 0x1000 + 1;

    for(uint64_t i = 0; i < pageCount; i++)
    {
        currentPageManager.MapMemory((void *)((uint64_t)TranslateToHighHalfMemoryAddress((page + i) * 0x1000)),
            (void *)((uint64_t)(page + i) * 0x1000),
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
    }

    newProcess = (ProcessInfo *)TranslateToHighHalfMemoryAddress((uint64_t)newProcess);

    page = (uint64_t)newProcess->stack / 0x1000;
    offset = (uint64_t)newProcess->stack % 0x1000;
    pageCount = (sizeof(newProcess->stack) + offset) / 0x1000 + 1;

    for(uint64_t i = 0; i < pageCount; i++)
    {
        currentPageManager.MapMemory((void *)(uint64_t)((page + i) * 0x1000),
            (void *)((uint64_t)TranslateToPhysicalMemoryAddress((page + i) * 0x1000)),
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE);

        //Do the same for the user
        userPageManager.MapMemory((void *)(uint64_t)((page + i) * 0x1000),
            (void *)((uint64_t)TranslateToPhysicalMemoryAddress((page + i) * 0x1000)),
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE);
    }

    memset(newProcess, 0, sizeof(ProcessInfo));

    newProcess->ID = ++processIDCounter;
    newProcess->permissionLevel = current->process->permissionLevel;
    newProcess->cwd = current->process->cwd;
    newProcess->rflags = 0x202;
    newProcess->cr3 = (uint64_t)pageTableFrame;

    if(current->process->name != NULL)
    {
        newProcess->name = strdup(current->process->name);
    }
    else
    {
        newProcess->name = "(null)";
    }

    for(uint64_t i = 0; i < SIGNAL_MAX; i++)
    {
        newProcess->sigHandlers[i].sa_handler = current->process->sigHandlers[i].sa_handler;
    }

    newProcess->rsp = interruptStack->stackPointer;

    if(IsHigherHalf(newProcess->rsp))
    {
        newProcess->rsp = TranslateToPhysicalMemoryAddress(newProcess->rsp);
    }

    newProcess->rip = interruptStack->instructionPointer;
    newProcess->gid = current->process->gid;
    newProcess->uid = current->process->uid;
    newProcess->sigprocmask = current->process->sigprocmask;
    newProcess->ppid = current->process->ID;

    auto pcb = scheduler->AddProcess(newProcess);

    memcpy(pcb->FXSAVE, current->FXSAVE, sizeof(pcb->FXSAVE));

    pcb->fsBase = current->fsBase;
    pcb->r10 = interruptStack->r10;
    pcb->r11 = interruptStack->r11;
    pcb->r12 = interruptStack->r12;
    pcb->r13 = interruptStack->r13;
    pcb->r14 = interruptStack->r14;
    pcb->r15 = interruptStack->r15;
    pcb->r8 = interruptStack->r8;
    pcb->r9 = interruptStack->r9;
    pcb->rax = interruptStack->rax;
    pcb->rbp = interruptStack->rbp;
    pcb->rbx = interruptStack->rbx;
    pcb->rcx = interruptStack->rcx;
    pcb->rdi = interruptStack->rdi;
    pcb->rdx = interruptStack->rdx;
    pcb->rip = newProcess->rip;
    pcb->rsi = interruptStack->rsi;
    pcb->rflags = current->rflags;
    pcb->state = newProcess->state = PROCESS_STATE_RUNNING;
    pcb->ss = current->ss;
    pcb->cs = current->cs;
    pcb->cr3 = newProcess->cr3;
    pcb->rsp = newProcess->rsp;

    currentPageManager.Duplicate(higherPageTableFrame);

    processes.push_back(newProcess);

    lock.Unlock();

    *child = newProcess->ID;

    uint64_t childOffset = (uint64_t)child % 0x1000;

    void *newVirt = userPageManager.PhysicalMemory((uint8_t *)child - childOffset);

    if(newVirt == NULL)
    {
        DEBUG_OUT("fork: newVirt is NULL!", 0);
    }
    else
    {
        pid_t *childPID = (pid_t *)((uint8_t *)TranslateToHighHalfMemoryAddress((uint64_t)newVirt) + childOffset);

        *childPID = 0;
    }

    //Set the return value for the new process
    pcb->rax = 0;

    DEBUG_OUT("Forking process %llu (%llu) cr3: %p", current->process->ID, newProcess->ID, newProcess->cr3);

    return 0;
}

ProcessInfo *ProcessManager::GetProcess(pid_t pid)
{
    Threading::ScopedLock Lock(lock);

    for(auto &process : processes)
    {
        if(process->ID == pid)
        {
            return process;
        }
    }

    return NULL;
}

frg::vector<ProcessInfo *, frg_allocator> ProcessManager::GetChildProcesses(pid_t ppid)
{
    Threading::ScopedLock Lock(lock);

    frg::vector<ProcessInfo *, frg_allocator> outValue;

    for(auto &process : processes)
    {
        if(process->ppid == ppid)
        {
            outValue.push_back(process);
        }
    }

    return outValue;
}

void ProcessManager::Kill(pid_t pid, int signal)
{
    if(signal < 0 || signal >= SIGNAL_MAX)
    {
        return;
    }

    ProcessInfo *process = GetProcess(pid);

    Threading::ScopedLock Lock(lock);

    if(process == NULL || process->state != PROCESS_STATE_DEAD)
    {
        return;
    }

    if(process->sigHandlers[signal].sa_flags & SA_SIGINFO)
    {
        //TODO: Support sa_sigaction
        return;
    }

    if(signal == SIGKILL)
    {
        return;
    }

    void *handler = (void *)process->sigHandlers[signal].sa_handler;

    if(handler == SIG_IGN)
    {
        return;
    }
    
    if(handler == SIG_DFL)
    {
        switch(signal)
        {
            case SIGSEGV:
            case SIGTERM:
            case SIGILL:
            case SIGFPE:
            case SIGINT:

                return;

            default:
                DEBUG_OUT("Unhandled signal: %s", signalname(signal));

                return;
        }
    }
}

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
#include "syscall/syscall.hpp"
#include "stacktrace/stacktrace.hpp"
#include "filesystems/VFS.hpp"
#include "tss/tss.hpp"
#include "Panic.hpp"
#include "kasan/kasan.hpp"
#include "user/UserAccess.hpp"
#include "smp/SMP.hpp"
#include "lapic/LAPIC.hpp"

void KernelTask()
{
    //Idle process
    for(;;)
    {
        asm volatile("hlt");
    }
}

#define UPDATE_PROCESS_ACTIVE_PERMISSIONS(pcb) \
    switch(pcb->activePermissionLevel)\
    {\
        case PROCESS_PERMISSION_KERNEL:\
            pcb->cs = (GDTKernelBaseSelector + 0x00);\
            pcb->ss = (GDTKernelBaseSelector + 0x08);\
            \
            break;\
            \
        case PROCESS_PERMISSION_USER:\
            pcb->cs = (GDTUserBaseSelector + 0x10) | 3;\
            pcb->ss = (GDTUserBaseSelector + 0x08) | 3;\
            \
            break;\
    }

#define UPDATETSS(task) \
    { \
        if(info->bsp) \
        { \
            bootstrapTSS.rsp0 = (uint64_t)&task->process->kernelStack[PROCESS_STACK_SIZE]; \
            bootstrapTSS.ist1 = (uint64_t)&task->process->istStack[PROCESS_STACK_SIZE]; \
        } \
        else \
        { \
            info->tss.rsp0 = (uint64_t)&task->process->kernelStack[PROCESS_STACK_SIZE]; \
            info->tss.ist1 = (uint64_t)&task->process->istStack[PROCESS_STACK_SIZE]; \
        } \
    }

#if DEBUG_PROCESSES
#define HANDLEFORK(task) \
    task->state = PROCESS_STATE_RUNNING; \
    \
    DEBUG_OUT("Initializing fork %p for process %i: rsp: %p; rip: %p; cr3: %p", task, task->process->ID, task->rsp, task->rip, task->cr3); \
    \
    lock.Unlock(); \
    \
    SwapTasks(task);
#else
#define HANDLEFORK(task) \
    task->state = PROCESS_STATE_RUNNING; \
    \
    lock.Unlock(); \
    \
    SwapTasks(task);
#endif

#if USE_KASAN
    #define MAPSTACK(stack) \
        page = (uint64_t)stack / 0x1000; \
        pageCount = PROCESS_STACK_PAGE_COUNT; \
        \
        for(uint64_t i = 0; i < pageCount; i++) \
        { \
            currentPageManager.MapMemory((void *)(uint64_t)((page + i) * 0x1000), \
                (void *)((uint64_t)TranslateToPhysicalMemoryAddress((page + i) * 0x1000)), \
                PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE); \
            \
            userPageManager.IdentityMap((void *)(uint64_t)TranslateToPhysicalMemoryAddress((page + i) * 0x1000), \
                PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE); \
        }\
        UnpoisonKasanShadow((void *)(page * 0x1000), PROCESS_STACK_PAGE_COUNT * 0x1000);
#else
    #define MAPSTACK(stack) \
        page = (uint64_t)stack / 0x1000; \
        pageCount = PROCESS_STACK_PAGE_COUNT; \
        \
        for(uint64_t i = 0; i < pageCount; i++) \
        { \
            currentPageManager.MapMemory((void *)(uint64_t)((page + i) * 0x1000), \
                (void *)((uint64_t)TranslateToPhysicalMemoryAddress((page + i) * 0x1000)), \
                PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE); \
            \
            userPageManager.IdentityMap((void *)(uint64_t)TranslateToPhysicalMemoryAddress((page + i) * 0x1000), \
                PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE); \
        }
#endif

#define SAVE_TASK_STATE(task, stack) \
        task->r10 = stack->r10; \
        task->r11 = stack->r11; \
        task->r12 = stack->r12; \
        task->r13 = stack->r13; \
        task->r14 = stack->r14; \
        task->r15 = stack->r15; \
        task->r8 = stack->r8; \
        task->r9 = stack->r9; \
        task->rax = stack->rax; \
        task->rbp = stack->rbp; \
        task->rbx = stack->rbx; \
        task->rcx = stack->rcx; \
        task->rdi = stack->rdi; \
        task->rdx = stack->rdx; \
        task->rsi = stack->rsi; \
        task->rsp = stack->stackPointer; \
        task->rip = stack->instructionPointer; \
        task->rflags = stack->cpuFlags; \
        \
        UPDATE_PROCESS_ACTIVE_PERMISSIONS(task);


#define LD_BASE 0x40000000
#define push(stack, value) *(--stack) = (value)

box<ProcessManager> processManager;
uint64_t processIDCounter = 0;

extern "C" void SwitchTasks(ProcessControlBlock* next);

void ProcessYieldIfAvailable()
{
    if(processManager.valid() == false)
    {
        return;
    }

    auto current = processManager->CurrentProcess();

    if(current == NULL || current->isValid == false)
    {
        return;
    }

    ProcessYield();
}

void SwitchProcess(InterruptStack *stack)
{
    DEBUG_OUT("SwitchProcess", "");

    LAPICTimerOneShot(100, (void *)SwitchProcess);

    processManager->SwitchProcess(stack);
}

ProcessManager::ProcessManager() : futexes(NULL)
{
}

bool ProcessManager::IsLocked()
{
    return lock.IsLocked();
}

void ProcessManager::SwitchProcess(InterruptStack *stack)
{
    lock.Lock();

    interrupts.DisableInterrupts();

    CPUInfo *info = CurrentCPUInfo();

    if(info == nullptr)
    {
        lock.Unlock();

        interrupts.EnableInterrupts();

        return;
    }

    IScheduler *scheduler = &info->scheduler;

    ProcessControlBlock *current = scheduler->CurrentThread();
    ProcessControlBlock *next = scheduler->NextThread();

    if(current == NULL || next == NULL)
    {
        lock.Unlock();

        interrupts.EnableInterrupts();

        return;
    }

    while(next->state == PROCESS_STATE_BLOCKED && next != current)
    {
        scheduler->Advance();

        next = scheduler->NextThread();
    }

    if(current == next || next->state == PROCESS_STATE_BLOCKED)
    {
        lock.Unlock();

        interrupts.EnableInterrupts();

        return;
    }

    if(current->state == PROCESS_STATE_RUNNING || current->state == PROCESS_STATE_BLOCKED)
    {
        SAVE_TASK_STATE(current, stack);
    }

    //Forked processes are added as the current process, so must init them properly
    if(current->state == PROCESS_STATE_FORKED)
    {
        HANDLEFORK(current);

        return;
    }
    else if(next->state == PROCESS_STATE_FORKED)
    {
        scheduler->Advance();

        HANDLEFORK(next);

        return;
    }

    if(next->state == PROCESS_STATE_NEEDS_INIT)
    {
        next->state = PROCESS_STATE_RUNNING;

#if DEBUG_PROCESSES
        DEBUG_OUT("Initializing task %p for process %i (tid: %i): rsp: %p; rip: %p; cr3: %p", next, next->process->ID, next->tid, next->rsp, next->rip, next->cr3);
#endif

        scheduler->Advance();

        lock.Unlock();

        SwapTasks(next);

        return;
    }

#if DEBUG_PROCESSES_EXTRA
    DEBUG_OUT("Switching tasks:\n\tID: %i\n\ttid: %i\n\tstate: %s\n\trsp: %p\n\trip: %p\n\tcr3: %p\n\tcs: 0x%x\n\tss: 0x%x\n"
        "next:\n\tID: %i\n\ttid: %i\n\tstate: %s\n\trsp: %p\n\trip: %p\n\tcr3: %p\n\tcs: 0x%x\n\tss: 0x%x",
        current->process->ID, current->tid, current->State().data(), current->rsp, current->rip, current->cr3, current->cs, current->ss,
        next->process->ID, next->tid, next->State().data(), next->rsp, next->rip, next->cr3, next->cs, next->ss);
#endif

    scheduler->Advance();

    lock.Unlock();

    SwapTasks(next);
}

void ProcessManager::SwapTasks(ProcessControlBlock *next)
{
#if DEBUG_PROCESSES_EXTRA
    DEBUG_OUT("Swapping to task with pid: %i tid: %i RIP %p RSP %p and CR3 %p", next->process->ID, next->tid, next->rip, next->rsp, next->cr3);
#endif

    LoadFSBase(next->fsBase);

    CPUInfo *info = CurrentCPUInfo();

    if(info == nullptr)
    {
        return;
    }

    UPDATE_PROCESS_ACTIVE_PERMISSIONS(next);

    UPDATETSS(next);

    SwitchTasks(next);
}

ProcessManager::ProcessPair *ProcessManager::CurrentProcess()
{
    lock.Lock();

    pid_t pid = 0;

    CPUInfo *info = CurrentCPUInfo();

    if(info == nullptr)
    {
        lock.Unlock();

        return nullptr;
    }

    IScheduler *scheduler = &info->scheduler;

    if(scheduler != NULL && scheduler->CurrentThread() != NULL)
    {
        pid = scheduler->CurrentThread()->process->ID;
    }

    lock.Unlock();

    auto current = GetProcess(pid);

    return current;
}

ProcessControlBlock *ProcessManager::CurrentThread()
{
    lock.Lock();

    ProcessControlBlock *pcb = NULL;

    CPUInfo *info = CurrentCPUInfo();

    if(info == nullptr)
    {
        lock.Unlock();

        return nullptr;
    }

    IScheduler *scheduler = &info->scheduler;

    if(scheduler != NULL && scheduler->CurrentThread() != NULL)
    {
        pcb = scheduler->CurrentThread();
    }

    lock.Unlock();

    return pcb;
}

ProcessControlBlock *ProcessManager::CreateFromEntryPoint(uint64_t entryPoint, const char *name, const char *cwd, uint64_t permissionLevel)
{
    lock.Lock();

    CPUInfo *info = CurrentCPUInfo();

    if(info == nullptr)
    {
        lock.Unlock();

        return nullptr;
    }

    IScheduler *scheduler = &info->scheduler;

    PageTable *pageTableFrame = (PageTable *)globalAllocator.RequestPage();

    PageTableManager currentPageManager;
    currentPageManager.p4 = (PageTable *)Registers::ReadCR3();

    currentPageManager.MapMemory((void *)TranslateToHighHalfMemoryAddress((uint64_t)pageTableFrame),
        pageTableFrame, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);

    PageTable *higherPageTableFrame = (PageTable *)TranslateToHighHalfMemoryAddress((uint64_t)pageTableFrame);

    UnpoisonKasanShadow(higherPageTableFrame, 0x1000);

    memset(higherPageTableFrame, 0, sizeof(PageTable));

    auto higherCurrentP4 = (PageTable *)TranslateToHighHalfMemoryAddress((uint64_t)currentPageManager.p4);
    
    for(uint64_t i = 256; i < 512; i++)
    {
        higherPageTableFrame->entries[i] = higherCurrentP4->entries[i];
    }

    PageTableManager userPageManager;
    userPageManager.p4 = pageTableFrame;

    auto processPageSize = sizeof(Process) / 0x1000 + 1;

    Process *newProcess = (Process *)globalAllocator.RequestPages(processPageSize);

    uint64_t page = (uint64_t)newProcess / 0x1000;
    uint64_t offset = (uint64_t)newProcess % 0x1000;

    uint64_t pageCount = (sizeof(Process) + offset) / 0x1000 + 1;

    for(uint64_t i = 0; i < pageCount; i++)
    {
        currentPageManager.MapMemory((void *)((uint64_t)TranslateToHighHalfMemoryAddress((page + i) * 0x1000)),
            (void *)((uint64_t)(page + i) * 0x1000),
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
    }

    newProcess = (Process *)TranslateToHighHalfMemoryAddress((uint64_t)newProcess);

    UnpoisonKasanShadow(newProcess, processPageSize * 0x1000);

    new (newProcess) Process();

    newProcess->ID = ++processIDCounter;
    newProcess->permissionLevel = permissionLevel;
    newProcess->cwd = cwd;
    newProcess->state = PROCESS_STATE_NEEDS_INIT;

    newProcess->kernelStack = (uint64_t *)TranslateToHighHalfMemoryAddress((uint64_t)globalAllocator.RequestPages(PROCESS_STACK_PAGE_COUNT));
    newProcess->istStack = (uint64_t *)TranslateToHighHalfMemoryAddress((uint64_t)globalAllocator.RequestPages(PROCESS_STACK_PAGE_COUNT));
    newProcess->stack = (uint64_t *)TranslateToHighHalfMemoryAddress((uint64_t)globalAllocator.RequestPages(PROCESS_STACK_PAGE_COUNT));

    MAPSTACK(newProcess->kernelStack);
    MAPSTACK(newProcess->istStack);
    MAPSTACK(newProcess->stack);

    newProcess->name = name;

    for(uint64_t i = 0; i < SIGNAL_MAX; i++)
    {
        newProcess->sigHandlers[i].sa_handler = SIG_DFL;
    }

    char **env = (char **)calloc(1, sizeof(char *[10]));

    newProcess->environment = env;

    uint64_t *stack = (uint64_t *)&newProcess->stack[PROCESS_STACK_SIZE];

    push(stack, entryPoint);

    newProcess->rsp = (uint64_t)stack;
    newProcess->rip = entryPoint;
    newProcess->rflags = 0x202;

    newProcess->cr3 = (uint64_t)pageTableFrame;

    newProcess->AddFD(PROCESS_FD_STDIN, new ProcessFDStdin());
    newProcess->AddFD(PROCESS_FD_STDOUT, new ProcessFDStdout());
    newProcess->AddFD(PROCESS_FD_STDERR, new ProcessFDStderr());

    auto pcb = scheduler->AddThread(newProcess, newProcess->rip, newProcess->rsp, newProcess->ID, true);

    pcb->activePermissionLevel = newProcess->permissionLevel;

    ProcessPair pair;

    pair.isValid = true;
    pair.info = newProcess;
    pair.threads.push_back(pcb);

    processes.push_back(pair);

#if DEBUG_PROCESSES
    DEBUG_OUT("Initializing entry point process at RIP %p; RSP: %p; CR3: %p", newProcess->rip, newProcess->rsp, newProcess->cr3);
#endif

    lock.Unlock();

    return pcb;
}

ProcessControlBlock *ProcessManager::LoadImage(const void *image, const char *name, const char **argv, const char **envp, const char *cwd,
    uint64_t permissionLevel, uint64_t IDOverride)
{
    lock.Lock();

    CPUInfo *info = CurrentCPUInfo();

    if(info == nullptr)
    {
        lock.Unlock();

        return nullptr;
    }

    IScheduler *scheduler = &info->scheduler;

    PageTable *pageTableFrame = (PageTable *)globalAllocator.RequestPage();
    PageTable *currentTable = (PageTable *)Registers::ReadCR3();
    PageTableManager currentPageManager;
    currentPageManager.p4 = currentTable;

    currentPageManager.MapMemory((void *)TranslateToHighHalfMemoryAddress((uint64_t)pageTableFrame), pageTableFrame,
        PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);

    PageTable *higherPageTableFrame = (PageTable *)TranslateToHighHalfMemoryAddress((uint64_t)pageTableFrame);

    UnpoisonKasanShadow(higherPageTableFrame, 0x1000);

    memset(higherPageTableFrame, 0, sizeof(PageTable));

    auto higherCurrentP4 = (PageTable *)TranslateToHighHalfMemoryAddress((uint64_t)currentPageManager.p4);
    
    for(uint64_t i = 256; i < 512; i++)
    {
        higherPageTableFrame->entries[i] = higherCurrentP4->entries[i];
    }

    PageTableManager userPageManager;
    userPageManager.p4 = pageTableFrame;

    auto processPageSize = sizeof(Process) / 0x1000 + 1;

    Process *newProcess = (Process *)globalAllocator.RequestPages(processPageSize);

    uint64_t page = (uint64_t)newProcess / 0x1000;
    uint64_t offset = (uint64_t)newProcess % 0x1000;

    uint64_t pageCount = (sizeof(Process) + offset) / 0x1000 + 1;

    for(uint64_t i = 0; i < pageCount; i++)
    {
        currentPageManager.MapMemory((void *)((uint64_t)TranslateToHighHalfMemoryAddress((page + i) * 0x1000)),
            (void *)((uint64_t)(page + i) * 0x1000),
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
    }

    newProcess = (Process *)TranslateToHighHalfMemoryAddress((uint64_t)newProcess);

    UnpoisonKasanShadow(newProcess, processPageSize * 0x1000);

    userPageManager.MapMemory((void *)TranslateToHighHalfMemoryAddress((uint64_t)pageTableFrame), pageTableFrame,
        PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE);

    new (newProcess) Process();

    newProcess->ID = IDOverride != 0 ? IDOverride : ++processIDCounter;
    newProcess->permissionLevel = permissionLevel;
    newProcess->cwd = cwd;
    newProcess->state = PROCESS_STATE_NEEDS_INIT;

    newProcess->name = name;

    newProcess->kernelStack = (uint64_t *)TranslateToHighHalfMemoryAddress((uint64_t)globalAllocator.RequestPages(PROCESS_STACK_PAGE_COUNT));
    newProcess->istStack = (uint64_t *)TranslateToHighHalfMemoryAddress((uint64_t)globalAllocator.RequestPages(PROCESS_STACK_PAGE_COUNT));
    newProcess->stack = (uint64_t *)TranslateToHighHalfMemoryAddress((uint64_t)globalAllocator.RequestPages(PROCESS_STACK_PAGE_COUNT));

    MAPSTACK(newProcess->kernelStack);
    MAPSTACK(newProcess->istStack);
    MAPSTACK(newProcess->stack);

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
#if DEBUG_PROCESSES
            DEBUG_OUT("Found LD for process: %s", ldPath);
#endif

            int error = 0;

            FILE_HANDLE ldHandle = vfs->OpenFile(ldPath, O_RDONLY, newProcess, &error);

            if(vfs->FileType(ldHandle) != FILE_HANDLE_FILE || error != 0)
            {
#if DEBUG_PROCESSES
                DEBUG_OUT("Failed to load ld binary at %s", ldPath);
#endif

                lock.Unlock();

                //TODO: Cleanup

                return NULL;
            }
            else
            {
                uint64_t fileSize = vfs->FileLength(ldHandle);

#if DEBUG_PROCESSES
                DEBUG_OUT("Loading LD with size %llu", fileSize);
#endif

                uint8_t *ldImage = new uint8_t[fileSize];

                if(vfs->ReadFile(ldHandle, ldImage, fileSize, &error) != fileSize || error != 0)
                {
#if DEBUG_PROCESSES
                    DEBUG_OUT("Failed to load ld binary at %s: I/O Error", ldPath);
#endif

                    lock.Unlock();

                    //TODO: Cleanup
                    return NULL;
                }

                Elf::Auxval ldAuxval;

                Elf::ElfHeader *ldElf = Elf::LoadElf(ldImage, LD_BASE, &ldAuxval);

                if(ldElf == NULL)
                {
#if DEBUG_PROCESSES
                    DEBUG_OUT("Invalid ld binary at %s", ldPath);
#endif

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

    memset(newProcess->stack, 0, PROCESS_STACK_SIZE);
    memset(newProcess->istStack, 0, PROCESS_STACK_SIZE);
    memset(newProcess->kernelStack, 0, PROCESS_STACK_SIZE);

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

    stackEnd = TranslateToPhysicalMemoryAddress(stackEnd);

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

#if DEBUG_PROCESSES
    DEBUG_OUT("Initializing process %i (IDOverride: %i) at RIP %p auxval entry: %p RSP: %p; CR3: %p",
        newProcess->ID, IDOverride, rip, auxval.entry, newProcess->rsp, newProcess->cr3);
#endif

    auto pcb = scheduler->AddThread(newProcess, newProcess->rip, newProcess->rsp, newProcess->ID, true);

    pcb->activePermissionLevel = newProcess->permissionLevel;

    newProcess->AddFD(PROCESS_FD_STDIN, new ProcessFDStdin());
    newProcess->AddFD(PROCESS_FD_STDOUT, new ProcessFDStdout());
    newProcess->AddFD(PROCESS_FD_STDERR, new ProcessFDStderr());

    ProcessPair pair;

    pair.isValid = true;
    pair.info = newProcess;
    pair.threads.push_back(pcb);

    processes.push_back(pair);

    lock.Unlock();

    return pcb;
}

void ProcessManager::LoadFSBase(uint64_t base)
{
    Registers::WriteMSR(0xC0000100, base);
}

void ProcessManager::Exit(int exitCode, bool forceRemove)
{
    lock.Lock();

    CPUInfo *info = CurrentCPUInfo();

    if(info == nullptr)
    {
        lock.Unlock();

        return;
    }

    IScheduler *scheduler = &info->scheduler;

    if(scheduler->CurrentThread() != NULL)
    {
        auto pcb = scheduler->CurrentThread();

        if(forceRemove)
        {
#if DEBUG_PROCESSES
            DEBUG_OUT("Process %i is being force removed", pcb->process->ID);
#endif
            pcb->process->state = PROCESS_STATE_REMOVED;
        }
        else
        {
            pcb->process->state = PROCESS_STATE_DEAD;
            pcb->process->exitCode = exitCode;

#if DEBUG_PROCESSES
            DEBUG_OUT("Process %i exited with code %d", pcb->process->ID, exitCode);
#endif
        }

        pcb->process->DisposeFDs();

        scheduler->ExitProcess(pcb->process);

        scheduler->DumpThreadList();
    }

    auto pcb = scheduler->CurrentThread();

    while(pcb->state == PROCESS_STATE_BLOCKED)
    {
        auto next = scheduler->NextThread();

        pcb = next;

        if(pcb->state == PROCESS_STATE_BLOCKED)
        {
            scheduler->Advance();

            continue;
        }
    }

#if DEBUG_PROCESSES
    DEBUG_OUT("Swapping to thread %i (pid: %i, rip: %p, rsp: %p, cr3: %p)", pcb->tid, pcb->process->ID, pcb->process->rip, pcb->process->rsp, pcb->process->cr3);
#endif

    lock.Unlock();

    SwapTasks(pcb);
}

void ProcessManager::Sigaction(int signum, struct sigaction *act, struct sigaction *oldact)
{
    if(signum < 0 || signum >= SIGNAL_MAX)
    {
        return;
    }

    ProcessPair *currentProcess = CurrentProcess();

    if(currentProcess == NULL || currentProcess->isValid == false)
    {
        return;
    }

    lock.Lock();

    if(act)
    {
        currentProcess->info->sigHandlers[signum] = *act;
    }

    if(oldact)
    {
        *oldact = currentProcess->info->sigHandlers[signum];
    }

    lock.Unlock();
}

void ProcessManager::SetUID(uid_t uid)
{
    ProcessPair *process = CurrentProcess();

    ScopedLock Lock(lock);

    if(process == NULL || process->isValid == false)
    {
        return;
    }

    process->info->uid = uid;
}

uid_t ProcessManager::GetUID()
{
    ProcessPair *process = CurrentProcess();

    ScopedLock Lock(lock);

    if(process == NULL || process->isValid == false)
    {

        return 0;
    }

    return process->info->uid;
}

void ProcessManager::SetGID(pid_t pid, gid_t gid)
{
    ProcessPair *process = GetProcess(pid);

    ScopedLock Lock(lock);

    if(process == NULL || process->isValid == false)
    {
        return;
    }

    process->info->gid = gid;
}

uid_t ProcessManager::GetGID()
{
    ProcessPair *process = CurrentProcess();

    ScopedLock Lock(lock);

    if(process == NULL || process->isValid == false)
    {
        return 0;
    }

    return process->info->gid;
}

int32_t ProcessManager::Fork(InterruptStack *interruptStack, pid_t *child)
{
    lock.Lock();

    CPUInfo *info = CurrentCPUInfo();

    if(info == nullptr)
    {
        lock.Unlock();

        return 0;
    }

    IScheduler *scheduler = &info->scheduler;

    auto current = scheduler->CurrentThread();

    PageTable *pageTableFrame = (PageTable *)globalAllocator.RequestPage();
    PageTable *currentTable = (PageTable *)Registers::ReadCR3();
    PageTableManager currentPageManager;
    currentPageManager.p4 = currentTable;

    currentPageManager.MapMemory((void *)TranslateToHighHalfMemoryAddress((uint64_t)pageTableFrame), pageTableFrame,
        PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE);

    PageTable *higherPageTableFrame = (PageTable *)TranslateToHighHalfMemoryAddress((uint64_t)pageTableFrame);

    UnpoisonKasanShadow(higherPageTableFrame, 0x1000);

    memset(higherPageTableFrame, 0, sizeof(PageTable));

    PageTableManager userPageManager;
    userPageManager.p4 = pageTableFrame;

    auto processPageSize = sizeof(Process) / 0x1000 + 1;

    Process *newProcess = (Process *)globalAllocator.RequestPages(processPageSize);

    uint64_t page = (uint64_t)newProcess / 0x1000;
    uint64_t offset = (uint64_t)newProcess % 0x1000;

    uint64_t pageCount = (sizeof(Process) + offset) / 0x1000 + 1;

    for(uint64_t i = 0; i < pageCount; i++)
    {
        currentPageManager.MapMemory((void *)((uint64_t)TranslateToHighHalfMemoryAddress((page + i) * 0x1000)),
            (void *)((uint64_t)(page + i) * 0x1000),
            PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
    }

    newProcess = (Process *)TranslateToHighHalfMemoryAddress((uint64_t)newProcess);

    UnpoisonKasanShadow(newProcess, processPageSize * 0x1000);

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

    new (newProcess) Process();

    newProcess->ID = ++processIDCounter;
    newProcess->permissionLevel = current->process->permissionLevel;
    newProcess->cwd = current->process->cwd;
    newProcess->rflags = 0x202;
    newProcess->cr3 = (uint64_t)pageTableFrame;

    newProcess->kernelStack = (uint64_t *)TranslateToHighHalfMemoryAddress((uint64_t)globalAllocator.RequestPages(PROCESS_STACK_PAGE_COUNT));
    newProcess->istStack = (uint64_t *)TranslateToHighHalfMemoryAddress((uint64_t)globalAllocator.RequestPages(PROCESS_STACK_PAGE_COUNT));
    newProcess->stack = (uint64_t *)TranslateToHighHalfMemoryAddress((uint64_t)globalAllocator.RequestPages(PROCESS_STACK_PAGE_COUNT));

    MAPSTACK(newProcess->kernelStack);
    MAPSTACK(newProcess->istStack);
    MAPSTACK(newProcess->stack);

    memcpy(newProcess->stack, current->process->stack, PROCESS_STACK_SIZE);
    memcpy(newProcess->istStack, current->process->istStack, PROCESS_STACK_SIZE);
    memcpy(newProcess->kernelStack, current->process->kernelStack, PROCESS_STACK_SIZE);

    if(current->process->name.size() > 0)
    {
        newProcess->name = current->process->name;
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

    auto pcb = scheduler->AddThread(newProcess, newProcess->rip, newProcess->rsp, current->tid, true);

    newProcess->fds = current->process->fds;
    newProcess->pipes = current->process->pipes;
    newProcess->fdCounter = current->process->fdCounter;

    newProcess->IncreaseFDRefs();

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
    pcb->state = newProcess->state = PROCESS_STATE_FORKED;
    pcb->ss = current->ss;
    pcb->cs = current->cs;
    pcb->cr3 = newProcess->cr3;
    pcb->rsp = newProcess->rsp;

    currentPageManager.Duplicate(higherPageTableFrame);

    ProcessPair pair;

    pair.isValid = true;
    pair.info = newProcess;
    pair.threads.push_back(pcb);

    processes.push_back(pair);

    lock.Unlock();

    WriteUserObject(child, newProcess->ID);

    uint64_t childOffset = (uint64_t)child % 0x1000;

    void *newVirt = userPageManager.PhysicalMemory((uint8_t *)child - childOffset);

    if(newVirt == NULL)
    {
#if DEBUG_PROCESSES
        DEBUG_OUT("fork: newVirt is NULL!", 0);
#endif
    }
    else
    {
        pid_t *childPID = (pid_t *)((uint8_t *)TranslateToHighHalfMemoryAddress((uint64_t)newVirt) + childOffset);

        *childPID = 0;
    }

    //Set the return value for the new process
    pcb->rax = 0;

#if DEBUG_PROCESSES
    DEBUG_OUT("Forking process %i (%i) cr3: %p, cs: %llx, ss: %llx, permission: %llx", current->process->ID, newProcess->ID, newProcess->cr3,
        pcb->cs, pcb->ss, newProcess->permissionLevel);
#endif

    return 0;
}

ProcessControlBlock *ProcessManager::AddThread(uint64_t rip, uint64_t rsp)
{
    lock.Lock();

    CPUInfo *info = CurrentCPUInfo();

    if(info == nullptr)
    {
        lock.Unlock();

        return nullptr;
    }

    IScheduler *scheduler = &info->scheduler;

    auto current = scheduler->CurrentThread();
    auto thread = scheduler->AddThread(current->process, rip, rsp, ++processIDCounter, false);

    lock.Unlock();

    return thread;
}

ProcessManager::ProcessPair *ProcessManager::GetProcess(pid_t pid)
{
    ScopedLock Lock(lock);

    for(auto &pair : processes)
    {
        if(!pair.isValid || pair.info->state == PROCESS_STATE_DEAD || pair.info->state == PROCESS_STATE_REMOVED)
        {
            continue;
        }

        if(pair.info->ID == pid)
        {
            return &pair;
        }
    }

    return NULL;
}

vector<ProcessManager::ProcessPair> ProcessManager::GetChildProcesses(pid_t ppid)
{
    ScopedLock Lock(lock);

    vector<ProcessPair> outValue;

    for(auto &pair : processes)
    {
        if(!pair.isValid || pair.info->state == PROCESS_STATE_REMOVED)
        {
            continue;
        }

        if(pair.info->ppid == ppid)
        {
            outValue.push_back(pair);
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

    ProcessPair *process = GetProcess(pid);

    ScopedLock Lock(lock);

    if(process == NULL || process->info->state == PROCESS_STATE_DEAD || process->info->state == PROCESS_STATE_REMOVED)
    {
        return;
    }

    if(process->info->sigHandlers[signal].sa_flags & SA_SIGINFO)
    {
        //TODO: Support sa_sigaction
        return;
    }

    if(signal == SIGKILL)
    {
        return;
    }

    void *handler = (void *)process->info->sigHandlers[signal].sa_handler;

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

bool ProcessManager::RemoveFutex(FutexPair *futex)
{
    while(futex->threads != NULL)
    {
        RemoveFutexThread(futex, futex->threads->pcb);
    }

    auto pointer = futex->pointer;
    (void)pointer;

    if(futex == futexes)
    {
       auto next = futex->next;

        delete futex;

        futexes = next;

#if DEBUG_PROCESSES_EXTRA
        DEBUG_OUT("Futex: Deleted futex for pointer %p (first)", pointer);
#endif

        return true;
    }
    else
    {
        auto previous = futexes;

        while(previous->next != NULL && previous->next != futex)
        {
            previous = previous->next;
        }            

        if(previous->next != futex)
        {
            Panic("Failed to delete a futex pair!");
        }

        auto next = futex->next;

        delete futex;

        previous->next = next;

#if DEBUG_PROCESSES_EXTRA
        DEBUG_OUT("Futex: Deleted futex for pointer %p", pointer);
#endif
    }

    return false;
}

bool ProcessManager::RemoveFutexThread(FutexPair *futex, ProcessControlBlock *pcb)
{
    auto thread = futex->threads;

    while(thread->pcb != pcb && thread->next != NULL)
    {
        thread = thread->next;
    }

    if(thread->pcb != pcb)
    {
#if DEBUG_PROCESSES_EXTRA
        DEBUG_OUT("Futex: Failed to delete thread %p (pid: %i, tid: %i)", pcb, pcb->process->ID, pcb->tid);
#endif

        return false;
    }

    if(thread == futex->threads)
    {
        auto next = thread->next;

        futex->threads = next;

        delete thread;

#if DEBUG_PROCESSES_EXTRA
        DEBUG_OUT("Futex: Deleted thread at futex for pointer %p (first)", futex->pointer);
#endif

        return true;
    }
    else
    {
        auto previous = futex->threads;

        while(previous->next != NULL && previous->next != thread)
        {
            previous = previous->next;
        }

        if(previous->next == thread)
        {
            auto next = thread->next;

            previous->next = next;

            delete thread;

#if DEBUG_PROCESSES_EXTRA
            DEBUG_OUT("Futex: Deleted thread at futex for pointer %p", futex->pointer);
#endif

            return true;
        }
    }

    return false;
}

void ProcessManager::ExitThread()
{
    auto currentProcess = CurrentProcess();

    lock.Lock();

    CPUInfo *info = CurrentCPUInfo();

    if(info == nullptr)
    {
        lock.Unlock();

        return;
    }

    IScheduler *scheduler = &info->scheduler;

    if(scheduler->CurrentThread() != NULL)
    {
        auto pcb = scheduler->CurrentThread();

#if DEBUG_PROCESSES_EXTRA
        DEBUG_OUT("Thread %i for process %i is being terminated", pcb->tid, pcb->process->ID);
#endif

        if(pcb->isMainThread)
        {
            pcb->process->state = PROCESS_STATE_DEAD;

            pcb->process->exitCode = 0;

            pcb->process->DisposeFDs();

#if DEBUG_PROCESSES_EXTRA
            DEBUG_OUT("Main Thread terminated, terminating process", 0);
#endif

            scheduler->ExitProcess(pcb->process);

            auto futex = futexes;

            while(futex != NULL)
            {
                for(int i = currentProcess->threads.size() - 1; i >= 0; i--)
                {
                    if(currentProcess->threads[i] == NULL)
                    {
                        continue;
                    }

                    if(RemoveFutexThread(futex, currentProcess->threads[i]))
                    {
                        currentProcess->threads[i] = NULL;
                    }
                }

                if(futex->threads == NULL)
                {
                    RemoveFutex(futex);

                    futex = futexes;

                    continue;
                }

                futex = futex->next;
            }

            DumpFutexStats();
        }
        else
        {
            scheduler->ExitThread(pcb);

            auto futex = futexes;

            while(futex != NULL)
            {
                if(RemoveFutexThread(futex, pcb))
                {
                    for(int i = currentProcess->threads.size() - 1; i >= 0; i--)
                    {
                        if(currentProcess->threads[i] == pcb)
                        {
                            currentProcess->threads[i] = NULL;

                            break;
                        }
                    }

                    break;
                }

                if(futex->threads == NULL)
                {
                    RemoveFutex(futex);

                    futex = futexes;

                    continue;
                }

                futex = futex->next;
            }

            DumpFutexStats();
        }

        scheduler->DumpThreadList();
    }

    auto pcb = scheduler->CurrentThread();

    while(pcb->state == PROCESS_STATE_BLOCKED)
    {
        auto next = scheduler->NextThread();

        if(next->state == PROCESS_STATE_BLOCKED)
        {
            scheduler->Advance();

            pcb = next;

            continue;
        }

        pcb = next;
    }

#if DEBUG_PROCESSES_EXTRA
    DEBUG_OUT("Swapping to thread %i (pid: %i, rip: %p, rsp: %p, cr3: %p)", pcb->tid, pcb->process->ID, pcb->process->rip, pcb->process->rsp, pcb->process->cr3);
#endif

    lock.Unlock();

    SwapTasks(pcb);
}

void ProcessManager::FutexWait(int *pointer, int expected, InterruptStack *stack)
{
    if(*pointer != expected)
    {
#if DEBUG_PROCESSES_EXTRA
        DEBUG_OUT("Futex: wait: not blocking", 0);
#endif

        DumpFutexStats();

        return;
    }

    interrupts.DisableInterrupts();

    CPUInfo *info = CurrentCPUInfo();

    if(info == nullptr)
    {
        return;
    }

    IScheduler *scheduler = &info->scheduler;

    auto currentThread = CurrentThread();

#if DEBUG_PROCESSES_EXTRA
    DEBUG_OUT("Futex: Preparing to block current thread %p", currentThread);
#endif

    lock.Lock();

    #define ADDTHREAD(thread) \
        thread->next = NULL; \
        thread->pcb = currentThread; \
        \
        currentThread->state = PROCESS_STATE_BLOCKED;\

    if(futexes == NULL)
    {
        futexes = new FutexPair();

        futexes->next = NULL;
        futexes->pointer = pointer;

        futexes->threads = new FutexThread();

        ADDTHREAD(futexes->threads);

#if DEBUG_PROCESSES_EXTRA
        DEBUG_OUT("Futex: Added first thread to futexes", 0);
#endif
    }
    else
    {
        auto futex = futexes;
        bool found = false;

        do
        {
            if(futex->pointer == pointer)
            {
                found = true;

                break;
            }

        } while(futex->next != NULL);

        if(found)
        {
#if DEBUG_PROCESSES_EXTRA
            DEBUG_OUT("Futex: Found futex for pointer %p", pointer);
#endif

            auto thread = futex->threads;

            while(thread->next != NULL)
            {
                thread = thread->next;
            }

            thread->next = new FutexThread();

            ADDTHREAD(thread->next);
        }
        else
        {
#if DEBUG_PROCESSES_EXTRA
            DEBUG_OUT("Futex: Adding futex for pointer %p", pointer);
#endif

            auto newFutex = new FutexPair();
            newFutex->next = NULL;
            newFutex->pointer = pointer;
            newFutex->threads = new FutexThread();

            futex->next = newFutex;

            ADDTHREAD(newFutex->threads);
        }
    }

    DumpFutexStats();

    scheduler->Advance();

    auto pcb = scheduler->CurrentThread();

    while(pcb->state == PROCESS_STATE_BLOCKED)
    {
        scheduler->Advance();

        pcb = scheduler->CurrentThread();
    }

    SAVE_TASK_STATE(currentThread, stack);

    lock.Unlock();

#if DEBUG_PROCESSES_EXTRA
    DEBUG_OUT("Current thread %p (pid: %i, tid: %i) has been blocked, swapping to thread %p (pid: %i, tid: %i)",
        currentThread, currentThread->process->ID, currentThread->tid,
        pcb, pcb->process->ID, pcb->tid);
#endif

    SwapTasks(pcb);
}

void ProcessManager::FutexWake(int *pointer)
{
    ScopedLock lock(this->lock);

    if(futexes == NULL)
    {
#if DEBUG_PROCESSES_EXTRA
        DEBUG_OUT("Futex. No futexes initialized", 0);
#endif

        return;
    }

    auto futex = futexes;

    while(futex->next != NULL && futex->pointer != pointer)
    {
        futex = futex->next;
    }

    if(futex->pointer != pointer)
    {
#if DEBUG_PROCESSES_EXTRA
        DEBUG_OUT("Futex: wake from futex that doesn't exist", 0);
#endif

        return;
    }

    auto thread = futex->threads;

    thread->pcb->state = PROCESS_STATE_RUNNING;

#if DEBUG_PROCESSES_EXTRA
    DEBUG_OUT("Futex: Woke thread %i (pid: %i)", thread->pcb->tid, thread->pcb->process->ID);
#endif

    if(thread->next == NULL)
    {
        RemoveFutex(futex);
    }
    else
    {
        RemoveFutexThread(futex, thread->pcb);
    }

    DumpFutexStats();
}

void ProcessManager::DumpFutexStats()
{
#if DEBUG_PROCESSES_EXTRA
    DEBUG_OUT("Futex: Dumping futex stats", 0);

    auto futex = futexes;

    if(futex == NULL)
    {
        DEBUG_OUT("Futex: No futexes allocated", 0);

        return;
    }

    do
    {
        DEBUG_OUT("  Futex for %p:", futex->pointer);

        auto thread = futex->threads;

        if(thread == NULL)
        {
            DEBUG_OUT("    Invalid starting thread (corruption?)", 0);
        }
        else
        {
            do
            {
                DEBUG_OUT("    Thread %p (pid: %i, tid: %i)", thread->pcb, thread->pcb->process->ID, thread->pcb->tid);

                thread = thread->next;
            } while(thread != NULL);
        }

        futex = futex->next;
    } while(futex != NULL);

    scheduler->DumpThreadList();
#endif
}

void ProcessManager::AddProcessVMMap(void *virt, void *physical, uint64_t pages)
{
    auto process = CurrentProcess();

    ScopedLock lock(this->lock);

    ProcessVMMap map;
    map.virt = virt;
    map.physical = physical;
    map.pageCount = pages;

    process->info->vmMapping.push_back(map);
}

void ProcessManager::AddProcessVMMap(void *virt, const vector<void *> &pages)
{
    auto process = CurrentProcess();

    ScopedLock lock(this->lock);

    ProcessVMMap map;
    map.virt = virt;
    map.physical = NULL;
    map.pageCount = 0;
    map.pages = pages;

    process->info->vmMapping.push_back(map);
}

void ProcessManager::ClearProcessVMMap(void *virt, uint64_t pages)
{
    auto process = CurrentProcess();

    ScopedLock lock(this->lock);

    for(auto &entry : process->info->vmMapping)
    {
        if(entry.virt == NULL || entry.virt != virt)
        {
            continue;
        }

        PageTableManager pageTableManger;
        pageTableManger.p4 = (PageTable *)process->info->cr3;

        auto target = (uint64_t)virt;

        if(entry.pageCount == 0 && entry.pages.size() > 0)
        {
            for(uint64_t i = 0; i < entry.pages.size(); i++)
            {
                pageTableManger.UnmapMemory((void *)((uint64_t)target + i * 0x1000));

                PoisonKasanShadow((uint8_t *)virt + i * 0x1000, 0x1000);

                globalAllocator.FreePage(entry.pages[i]);
            }
        }
        else
        {
            for(uint64_t i = 0; i < pages; i++)
            {
                pageTableManger.UnmapMemory((void *)((uint64_t)target + i * 0x1000));
            }

            PoisonKasanShadow(entry.virt, entry.pageCount * 0x1000);

            globalAllocator.FreePages(entry.physical, entry.pageCount);
        }

        entry.virt = NULL;
    }
}

void ProcessManager::Wait()
{
    interrupts.DisableInterrupts();

    LAPICTimerOneShot(20000, (void *)::SwitchProcess);

    interrupts.EnableInterrupts();

    for(;;)
    {
        asm volatile("hlt");
    }

    __builtin_unreachable();
}

#pragma once

#include <stdint.h>
#include <signal.h>
#include "elf/elf.hpp"
#include "lock.hpp"
#include "interrupts/Interrupts.hpp"
#include "kernel.h"

constexpr int PROCESS_STACK_SIZE = 0x4000;

static_assert((PROCESS_STACK_SIZE * sizeof(uint64_t)) % 0x1000 == 0, "Misaligned process stack size");

constexpr int PROCESS_STACK_PAGE_COUNT = PROCESS_STACK_SIZE * sizeof(uint64_t) / 0x1000;

enum ProcessPermissionLevel
{
    PROCESS_PERMISSION_KERNEL,
    PROCESS_PERMISSION_USER
};

enum ProcessState
{
    PROCESS_STATE_NEEDS_INIT = 0,
    PROCESS_STATE_IDLE,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_FORKED,
    PROCESS_STATE_DEAD,
};

struct ProcessInfo
{
    uint64_t ID;
    uint64_t permissionLevel;
    uint64_t activePermissionLevel;
    string name;
    char **argv;
    char **environment;
    uint64_t *kernelStack;
    uint64_t *istStack;
    uint64_t *stack;
    uint64_t rip;
    uint64_t rsp;
    uint64_t cr3;
    uint64_t rflags;
    uint64_t sleepTicks;
    uint64_t fsBase;
    sigaction sigHandlers[SIGNAL_MAX];
    uid_t uid;
    gid_t gid;
    pid_t ppid;
    sigset_t sigprocmask;
    uint64_t state;
    int exitCode;

    string cwd;

    Elf::ElfHeader *elf;
};

struct ProcessControlBlock
{
    uint64_t ss;
    uint64_t rsp;
    uint64_t rflags;
    uint64_t cs;
    uint64_t rip;

    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    uint64_t rdi;

    uint64_t cr3;

    uint64_t fsBase;

    ProcessInfo *process;

    uint64_t state;

    ProcessControlBlock *next;

    inline string State() const
    {
        switch(state)
        {
            case PROCESS_STATE_DEAD:

                return "DEAD";

            case PROCESS_STATE_FORKED:

                return "FORKED";

            case PROCESS_STATE_IDLE:

                return "IDLE";

            case PROCESS_STATE_NEEDS_INIT:

                return "NEEDS INIT";

            case PROCESS_STATE_RUNNING:

                return "RUNNING";

            default:

                return "UNKNOWN (CORRUPTED?)";
        }
    }
};

class IScheduler
{
public:
    virtual ProcessControlBlock *CurrentProcess() = 0;
    virtual ProcessControlBlock *AddProcess(ProcessInfo *process) = 0;
    virtual ProcessControlBlock *NextProcess() = 0;
    virtual void Advance() = 0;
    virtual void DumpProcessList() = 0;
    virtual void ExitProcess(ProcessInfo *process) = 0;
};

class ProcessManager
{
public:
    struct ProcessPair;
private:
    IScheduler *scheduler;
    vector<ProcessPair> processes;
public:
    struct ProcessPair
    {
        ProcessInfo *info;
        ProcessControlBlock *pcb;
        bool isValid;
    };

    Threading::AtomicLock lock;

    ProcessManager(IScheduler *scheduler);

    ProcessPair *LoadImage(const void *image, const char *name, const char **argv, const char **envp, const char *cwd,
        uint64_t permissionLevel, uint64_t IDOverride = 0);
    ProcessPair *CreateFromEntryPoint(uint64_t entryPoint, const char *name, const char *cwd, uint64_t permissionLevel);

    ProcessPair *CurrentProcess();
    ProcessPair *GetProcess(pid_t pid);
    vector<ProcessPair> GetChildProcesses(pid_t ppid);

    void SwitchProcess(InterruptStack *stack, bool fromTimer);

    bool IsLocked();

    void LoadFSBase(uint64_t base);

    void Sigaction(int signum, sigaction *act, sigaction *oldact);

    void Kill(pid_t pid, int signal);

    void Exit(int exitCode, bool forceRemove = false);

    void SetUID(pid_t pid, uid_t uid);

    uid_t GetUID(pid_t pid);

    void SetGID(pid_t pid, gid_t gid);

    gid_t GetGID(pid_t pid);

    int32_t Fork(InterruptStack *interruptStack, pid_t *child);
};

extern ProcessManager *globalProcessManager;

extern "C" void ProcessYield();

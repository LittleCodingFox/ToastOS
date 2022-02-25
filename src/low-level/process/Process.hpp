#pragma once

#include <stdint.h>
#include <signal.h>
#include "elf/elf.hpp"
#include "threading/lock.hpp"
#include "interrupts/Interrupts.hpp"
#include "filesystems/VFS.hpp"
#include "pipe/pipe.hpp"
#include "kernel.h"

extern "C" void ProcessYield();

constexpr int PROCESS_STACK_SIZE = 0x4000;

static_assert((PROCESS_STACK_SIZE * sizeof(uint64_t)) % 0x1000 == 0, "Misaligned process stack size");

constexpr int PROCESS_STACK_PAGE_COUNT = PROCESS_STACK_SIZE * sizeof(uint64_t) / 0x1000;

enum ProcessFDType
{
    PROCESS_FD_UNKNOWN,
    PROCESS_FD_HANDLE,
    PROCESS_FD_PIPE,
    PROCESS_FD_STDIN,
    PROCESS_FD_STDOUT,
    PROCESS_FD_STDERR
};

class IProcessFD
{
protected:
    int refCount;
public:
    IProcessFD() : refCount(1) {}
    virtual ~IProcessFD() {}
    virtual uint64_t Write(const void *buffer, uint64_t size, int *error) = 0;
    virtual uint64_t Read(void *buffer, uint64_t size, int *error) = 0;
    virtual int64_t Seek(uint64_t offset, int whence, int *error) = 0;
    virtual dirent *ReadEntries() = 0;
    virtual struct stat Stat(int *error) = 0;
    virtual void Close() = 0;

    int RefCount()
    {
        return refCount;
    }

    void IncreaseRef()
    {
        refCount++;
    }
};

struct ProcessPipe
{
    Pipe *pipe;
    bool isValid;

    ProcessPipe() : pipe(NULL), isValid(false) {}
};

struct ProcessFD
{
    int fd;
    int type;
    bool isValid;
    IProcessFD *impl;

    ProcessFD() : fd(0), type(PROCESS_FD_UNKNOWN), isValid(false), impl(NULL) {}
    ProcessFD(int fd, int type, bool isValid, IProcessFD *impl) : fd(fd), type(type), isValid(isValid), impl(impl) {}
};

#include "fd/ProcessFDPipe.hpp"
#include "fd/ProcessFDStderr.hpp"
#include "fd/ProcessFDStdin.hpp"
#include "fd/ProcessFDStdout.hpp"
#include "fd/ProcessFDVFS.hpp"

enum ProcessPermissionLevel
{
    PROCESS_PERMISSION_KERNEL,
    PROCESS_PERMISSION_USER
};

enum ProcessState
{
    PROCESS_STATE_NEEDS_INIT = 0,
    PROCESS_STATE_IDLE,
    PROCESS_STATE_BLOCKED,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_FORKED,
    PROCESS_STATE_DEAD,
    PROCESS_STATE_REMOVED
};

struct ProcessVMMap
{
    void *virt;
    void *physical;
    vector<void *> pages;
    uint64_t pageCount;
};

struct Process
{
    pid_t ID;
    uint64_t permissionLevel;
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
    struct sigaction sigHandlers[SIGNAL_MAX];
    vector<ProcessFD> fds;
    vector<ProcessVMMap> vmMapping;
    vector<ProcessPipe> pipes;

    int fdCounter;

    uid_t uid;
    gid_t gid;
    pid_t ppid;

    sigset_t sigprocmask;
    uint64_t state;
    int exitCode;
    bool didWaitPID;

    string cwd;

    Elf::ElfHeader *elf;

    AtomicLock lock;

    Process() : ID(0), permissionLevel(0), argv(NULL), environment(NULL), kernelStack(NULL), istStack(NULL), stack(NULL),
        rip(0), rsp(0), cr3(0), rflags(0), sleepTicks(0), fdCounter(0), uid(0), gid(0), ppid(0), sigprocmask(0), state(PROCESS_STATE_NEEDS_INIT),
        exitCode(0), didWaitPID(false), elf(NULL) {}

    int AddFD(int type, IProcessFD *impl);
    ProcessFD *GetFD(int fd);
    void CreatePipe(int *fds);
    int DuplicateFD(int fd, size_t arg);
    int DuplicateFD(int fd, int newfd);
    void CloseFD(int fd);
    void IncreaseFDRefs();
    void DisposeFDs();
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

    Process *process;

    uint64_t state;

    pid_t tid;

    uint64_t activePermissionLevel;

    bool isMainThread;

    ProcessControlBlock *next;

    inline string State() const
    {
        switch(state)
        {
            case PROCESS_STATE_BLOCKED:

                return "BLOCKED";

            case PROCESS_STATE_DEAD:

                return "DEAD";

            case PROCESS_STATE_FORKED:

                return "FORKED";

            case PROCESS_STATE_IDLE:

                return "IDLE";

            case PROCESS_STATE_NEEDS_INIT:

                return "NEEDS INIT";

            case PROCESS_STATE_REMOVED:

                return "REMOVED";

            case PROCESS_STATE_RUNNING:

                return "RUNNING";

            default:

                return "UNKNOWN (CORRUPTED?)";
        }
    }
};

struct FutexThread
{
    ProcessControlBlock *pcb;

    FutexThread *next;
};

struct FutexPair
{
    int *pointer;

    FutexThread *threads;

    FutexPair *next;
};

class IScheduler
{
public:
    virtual ProcessControlBlock *CurrentThread() = 0;
    virtual ProcessControlBlock *AddThread(Process *process, uint64_t rip, uint64_t rsp, pid_t tid, bool isMainThread) = 0;
    virtual ProcessControlBlock *NextThread() = 0;
    virtual void Advance() = 0;
    virtual void DumpThreadList() = 0;
    virtual void ExitProcess(Process *process) = 0;
    virtual void ExitThread(ProcessControlBlock *pcb) = 0;
};

class ProcessManager
{
public:
    struct ProcessPair;
private:
    IScheduler *scheduler;
    vector<ProcessPair> processes;
    FutexPair *futexes;

    void SwapTasks(ProcessControlBlock *next);
    bool RemoveFutexThread(FutexPair *futex, ProcessControlBlock *thread);
    bool RemoveFutex(FutexPair *futex);
    void DumpFutexStats();
public:
    struct ProcessPair
    {
        Process *info;
        vector<ProcessControlBlock *> threads;
        bool isValid;
    };

    AtomicLock lock;

    ProcessManager(IScheduler *scheduler);

    ProcessControlBlock *LoadImage(const void *image, const char *name, const char **argv, const char **envp, const char *cwd,
        uint64_t permissionLevel, uint64_t IDOverride = 0);
    ProcessControlBlock *CreateFromEntryPoint(uint64_t entryPoint, const char *name, const char *cwd, uint64_t permissionLevel);

    ProcessPair *CurrentProcess();
    ProcessControlBlock *CurrentThread();
    ProcessPair *GetProcess(pid_t pid);
    vector<ProcessPair> GetChildProcesses(pid_t ppid);

    void SwitchProcess(InterruptStack *stack, bool fromTimer);

    bool IsLocked();

    void LoadFSBase(uint64_t base);

    void Sigaction(int signum, struct sigaction *act, struct sigaction *oldact);

    void Kill(pid_t pid, int signal);

    void Exit(int exitCode, bool forceRemove = false);

    void ExitThread();

    void FutexWait(int *pointer, int expected, InterruptStack *stack);

    void FutexWake(int *pointer);

    void AddProcessVMMap(void *virt, void *physical, uint64_t pages);

    void AddProcessVMMap(void *virt, const vector<void *> &pages);

    void ClearProcessVMMap(void *virt, uint64_t pages);

    ProcessControlBlock *AddThread(uint64_t rip, uint64_t rsp);

    void SetUID(uid_t uid);

    uid_t GetUID();

    void SetGID(pid_t pid, gid_t gid);

    gid_t GetGID();

    int32_t Fork(InterruptStack *interruptStack, pid_t *child);
};

extern ProcessManager *globalProcessManager;

#include "syscall.hpp"
#include "process/Process.hpp"
#include "registers/Registers.hpp"
#include "gdt/gdt.hpp"
#include "debug.hpp"
#include "lock.hpp"

typedef int64_t (*SyscallPointer)(InterruptStack *stack);

int64_t KNotImplemented(InterruptStack *stack);
int64_t SyscallWrite(InterruptStack *stack);
int64_t SyscallRead(InterruptStack *stack);
int64_t SyscallClose(InterruptStack *stack);
int64_t SyscallOpen(InterruptStack *stack);
int64_t SyscallSeek(InterruptStack *stack);
int64_t SyscallAnonAlloc(InterruptStack *stack);
int64_t SyscallVMMap(InterruptStack *stack);
int64_t SyscallVMUnmap(InterruptStack *stack);
int64_t SyscallTCBSet(InterruptStack *stack);
int64_t SyscallSigaction(InterruptStack *stack);
int64_t SyscallGetPID(InterruptStack *stack);
int64_t SyscallKill(InterruptStack *stack);
int64_t SyscallIsATTY(InterruptStack *stack);
int64_t SyscallStat(InterruptStack *stack);
int64_t SyscallFStat(InterruptStack *stack);
int64_t SyscallPanic(InterruptStack *stack);
int64_t SyscallReadEntries(InterruptStack *stack);
int64_t SyscallExit(InterruptStack *stack);
int64_t SyscallClock(InterruptStack *stack);
int64_t SyscallGetUID(InterruptStack *stack);
int64_t SyscallSetUID(InterruptStack *stack);
int64_t SyscallGetGID(InterruptStack *stack);
int64_t SyscallSigprocMask(InterruptStack *stack);
int64_t SyscallFcntl(InterruptStack *stack);
int64_t SyscallLog(InterruptStack *stack);
int64_t SyscallFork(InterruptStack *stack);
int64_t SyscallWaitPID(InterruptStack *stack);
int64_t SyscallGetPPID(InterruptStack *stack);
int64_t SyscallSetGraphicsType(InterruptStack *stack);
int64_t SyscallSetGraphicsBuffer(InterruptStack *stack);
int64_t SyscallGetGraphicsSize(InterruptStack *stack);
int64_t SyscallExecve(InterruptStack *stack);
int64_t SyscallPollInput(InterruptStack *stack);
int64_t SyscallCWD(InterruptStack *stack);
int64_t SyscallCHDir(InterruptStack *stack);
int64_t SyscallSpawnThread(InterruptStack *stack);
int64_t SyscallYield(InterruptStack *stack);
int64_t SyscallExitThread(InterruptStack *stack);
int64_t SyscallGetTID(InterruptStack *stack);
int64_t SyscallFutexWait(InterruptStack *stack);
int64_t SyscallFutexWake(InterruptStack *stack);
int64_t SyscallSetKBLayout(InterruptStack *stack);
int64_t SyscallPipe(InterruptStack *stack);
int64_t SyscallDup2(InterruptStack *stack);

SyscallPointer syscallHandlers[] =
{
    [0] = (SyscallPointer)KNotImplemented,
    [SYSCALL_READ] = (SyscallPointer)SyscallRead,
    [SYSCALL_WRITE] = (SyscallPointer)SyscallWrite,
    [SYSCALL_OPEN] = (SyscallPointer)SyscallOpen,
    [SYSCALL_CLOSE] = (SyscallPointer)SyscallClose,
    [SYSCALL_SEEK] = (SyscallPointer)SyscallSeek,
    [SYSCALL_ANON_ALLOC] = (SyscallPointer)SyscallAnonAlloc,
    [SYSCALL_VM_MAP] = (SyscallPointer)SyscallVMMap,
    [SYSCALL_VM_UNMAP] = (SyscallPointer)SyscallVMUnmap,
    [SYSCALL_TCB_SET] = (SyscallPointer)SyscallTCBSet,
    [SYSCALL_SIGACTION] = (SyscallPointer)SyscallSigaction,
    [SYSCALL_GETPID] = (SyscallPointer)SyscallGetPID,
    [SYSCALL_KILL] = (SyscallPointer)SyscallKill,
    [SYSCALL_ISATTY] = (SyscallPointer)SyscallIsATTY,
    [SYSCALL_FSTAT] = (SyscallPointer)SyscallFStat,
    [SYSCALL_STAT] = (SyscallPointer)SyscallStat,
    [SYSCALL_PANIC] = (SyscallPointer)SyscallPanic,
    [SYSCALL_READ_ENTRIES] = (SyscallPointer)SyscallReadEntries,
    [SYSCALL_EXIT] = (SyscallPointer)SyscallExit,
    [SYSCALL_CLOCK] = (SyscallPointer)SyscallClock,
    [SYSCALL_GETUID] = (SyscallPointer)SyscallGetUID,
    [SYSCALL_SETUID] = (SyscallPointer)SyscallSetUID,
    [SYSCALL_GETGID] = (SyscallPointer)SyscallGetGID,
    [SYSCALL_SIGPROCMASK] = (SyscallPointer)SyscallSigprocMask,
    [SYSCALL_FCNTL] = (SyscallPointer)SyscallFcntl,
    [SYSCALL_LOG] = (SyscallPointer)SyscallLog,
    [SYSCALL_FORK] = (SyscallPointer)SyscallFork,
    [SYSCALL_WAITPID] = (SyscallPointer)SyscallWaitPID,
    [SYSCALL_GETPPID] = (SyscallPointer)SyscallGetPPID,
    [SYSCALL_SETGRAPHICSTYPE] = (SyscallPointer)SyscallSetGraphicsType,
    [SYSCALL_GETGRAPHICSSIZE] = (SyscallPointer)SyscallGetGraphicsSize,
    [SYSCALL_SETGRAPHICSBUFFER] = (SyscallPointer)SyscallSetGraphicsBuffer,
    [SYSCALL_EXECVE] = (SyscallPointer)SyscallExecve,
    [SYSCALL_POLLINPUT] = (SyscallPointer)SyscallPollInput,
    [SYSCALL_CWD] = (SyscallPointer)SyscallCWD,
    [SYSCALL_CHDIR] = (SyscallPointer)SyscallCHDir,
    [SYSCALL_SPAWN_THREAD] = (SyscallPointer)SyscallSpawnThread,
    [SYSCALL_YIELD] = (SyscallPointer)SyscallYield,
    [SYSCALL_THREAD_EXIT] = (SyscallPointer)SyscallExitThread,
    [SYSCALL_GET_TID] = (SyscallPointer)SyscallGetTID,
    [SYSCALL_FUTEX_WAIT] = (SyscallPointer)SyscallFutexWait,
    [SYSCALL_FUTEX_WAKE] = (SyscallPointer)SyscallFutexWake,
    [SYSCALL_SETKBLAYOUT] = (SyscallPointer)SyscallSetKBLayout,
    [SYSCALL_PIPE] = (SyscallPointer)SyscallPipe,
    [SYSCALL_DUP2] = (SyscallPointer)SyscallDup2,
};

void SyscallHandler(InterruptStack *stack)
{
    if(stack->rdi <= sizeof(syscallHandlers) / sizeof(SyscallPointer) && syscallHandlers[stack->rdi] != NULL)
    {
        auto current = globalProcessManager->CurrentThread();

        current->activePermissionLevel = PROCESS_PERMISSION_KERNEL;

        auto handler = syscallHandlers[stack->rdi];
        auto result = handler(stack);

        stack->rax = result;

        current->activePermissionLevel = PROCESS_PERMISSION_USER;
    }
}

int64_t KNotImplemented(InterruptStack *stack)
{
    DEBUG_OUT("Syscall: KNotImplemented", 0);

    return -1;
}

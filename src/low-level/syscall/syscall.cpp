#include "syscall.hpp"
#include "process/Process.hpp"
#include "registers/Registers.hpp"
#include "gdt/gdt.hpp"
#include "debug.hpp"
#include "threading/lock.hpp"

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
int64_t SyscallGetRandom(InterruptStack *stack);
int64_t SyscallGetHostname(InterruptStack *stack);
int64_t Syscallioctl(InterruptStack *stack);
int64_t Syscallpselect(InterruptStack *stack);
int64_t SyscallTTYName(InterruptStack *stack);
int64_t SyscallTCGetAttr(InterruptStack *stack);
int64_t SyscallTCSetAttr(InterruptStack *stack);
int64_t SyscallTCFlow(InterruptStack *stack);
int64_t SyscallReadLink(InterruptStack *stack);
int64_t SyscallSysInfo(InterruptStack *stack);
int64_t SyscallGetRusage(InterruptStack *stack);
int64_t SyscallGetRLimit(InterruptStack *stack);
int64_t SyscallFchdir(InterruptStack *stack);
int64_t SyscallUname(InterruptStack *stack);

SyscallPointer syscallHandlers[] =
{
    (SyscallPointer)KNotImplemented,
    (SyscallPointer)SyscallRead,
    (SyscallPointer)SyscallWrite,
    (SyscallPointer)SyscallOpen,
    (SyscallPointer)SyscallClose,
    (SyscallPointer)SyscallSeek,
    (SyscallPointer)SyscallAnonAlloc,
    (SyscallPointer)KNotImplemented, //TODO: Anon Free
    (SyscallPointer)SyscallVMMap,
    (SyscallPointer)SyscallVMUnmap,
    (SyscallPointer)SyscallTCBSet,
    (SyscallPointer)SyscallSigaction,
    (SyscallPointer)SyscallGetPID,
    (SyscallPointer)SyscallKill,
    (SyscallPointer)SyscallIsATTY,
    (SyscallPointer)SyscallFStat,
    (SyscallPointer)SyscallStat,
    (SyscallPointer)SyscallPanic,
    (SyscallPointer)SyscallReadEntries,
    (SyscallPointer)SyscallExit,
    (SyscallPointer)SyscallClock,
    (SyscallPointer)SyscallGetUID,
    (SyscallPointer)SyscallSetUID,
    (SyscallPointer)SyscallGetGID,
    (SyscallPointer)SyscallSigprocMask,
    (SyscallPointer)SyscallFcntl,
    (SyscallPointer)SyscallLog,
    (SyscallPointer)SyscallFork,
    (SyscallPointer)SyscallWaitPID,
    (SyscallPointer)SyscallGetPPID,
    (SyscallPointer)SyscallSetGraphicsType,
    (SyscallPointer)SyscallGetGraphicsSize,
    (SyscallPointer)SyscallSetGraphicsBuffer,
    (SyscallPointer)SyscallExecve,
    (SyscallPointer)SyscallPollInput,
    (SyscallPointer)SyscallCWD,
    (SyscallPointer)SyscallCHDir,
    (SyscallPointer)KNotImplemented, //TODO: Sleep
    (SyscallPointer)SyscallSpawnThread,
    (SyscallPointer)SyscallYield,
    (SyscallPointer)SyscallExitThread,
    (SyscallPointer)SyscallGetTID,
    (SyscallPointer)SyscallFutexWait,
    (SyscallPointer)SyscallFutexWake,
    (SyscallPointer)SyscallSetKBLayout,
    (SyscallPointer)SyscallPipe,
    (SyscallPointer)SyscallDup2,
    (SyscallPointer)SyscallGetRandom,
    (SyscallPointer)SyscallGetHostname,
    (SyscallPointer)Syscallioctl,
    (SyscallPointer)Syscallpselect,
    (SyscallPointer)SyscallTTYName,
    (SyscallPointer)SyscallTCGetAttr,
    (SyscallPointer)SyscallTCSetAttr,
    (SyscallPointer)SyscallTCFlow,
    (SyscallPointer)SyscallReadLink,
    (SyscallPointer)SyscallSysInfo,
    (SyscallPointer)SyscallGetRusage,
    (SyscallPointer)SyscallGetRLimit,
    (SyscallPointer)SyscallFchdir,
    (SyscallPointer)SyscallUname,
};

void SyscallHandler(InterruptStack *stack)
{
    if(stack->rdi <= sizeof(syscallHandlers) / sizeof(SyscallPointer) && syscallHandlers[stack->rdi] != NULL)
    {
        auto current = processManager->CurrentThread();

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

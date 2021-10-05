#include "syscall.hpp"
#include "process/Process.hpp"
#include "registers/Registers.hpp"
#include "gdt/gdt.hpp"
#include "debug.hpp"
#include "lock.hpp"

extern "C" void syscallHandler();

typedef int64_t (*SyscallPointer)(InterruptStack *stack);

Threading::AtomicLock syscallLock;

bool PerformingSyscall()
{
    return syscallLock.IsLocked();
}

void SyscallLock()
{
    syscallLock.Lock();
}

void SyscallUnlock()
{
    syscallLock.Unlock();
}

int64_t KNotImplemented(InterruptStack *stack);
int64_t SyscallWrite(InterruptStack *stack);
int64_t SyscallRead(InterruptStack *stack);
int64_t SyscallClose(InterruptStack *stack);
int64_t SyscallOpen(InterruptStack *stack);
int64_t SyscallSeek(InterruptStack *stack);
int64_t SyscallAnonAlloc(InterruptStack *stack);
int64_t SyscallVMMap(InterruptStack *stack);

SyscallPointer syscallHandlers[] =
{
    [0] = (SyscallPointer)KNotImplemented,
    [SYSCALL_WRITE] = (SyscallPointer)SyscallWrite,
    [SYSCALL_READ] = (SyscallPointer)SyscallRead,
    [SYSCALL_OPEN] = (SyscallPointer)SyscallOpen,
    [SYSCALL_CLOSE] = (SyscallPointer)SyscallClose,
    [SYSCALL_SEEK] = (SyscallPointer)SyscallSeek,
    [SYSCALL_ANON_ALLOC] = (SyscallPointer)SyscallAnonAlloc,
    [SYSCALL_VM_MAP] = (SyscallPointer)SyscallVMMap
};

void SyscallHandler(InterruptStack *stack)
{
    DEBUG_OUT("SYSCALL %llu", stack->rdi);

    if(stack->rdi <= sizeof(syscallHandlers) / sizeof(SyscallPointer) && syscallHandlers[stack->rdi] != NULL)
    {
        ProcessInfo *process = globalProcessManager->CurrentProcess();

        bool needsPermissionChange = process->permissionLevel == PROCESS_PERMISSION_USER;

        if(needsPermissionChange)
        {
            process->permissionLevel = PROCESS_PERMISSION_KERNEL;
        }

        auto handler = syscallHandlers[stack->rdi];

        auto result = handler(stack);

        stack->rax = result;

        if(needsPermissionChange)
        {
            process->permissionLevel = PROCESS_PERMISSION_USER;
        }
    }
}

int64_t KNotImplemented(InterruptStack *stack)
{
    DEBUG_OUT("Syscall: KNotImplemented", 0);

    return -1;
}

void InitializeSyscalls()
{
    /*
    Registers::WriteMSR(Registers::IA32_EFER, Registers::ReadMSR(Registers::IA32_EFER) | 1);

    uint64_t star = Registers::ReadMSR(Registers::IA32_STAR);
    star |= (uint64_t)GDTKernelBaseSelector << 32;
    star |= (uint64_t)GDTUserBaseSelector << 48;

    Registers::WriteMSR(Registers::IA32_STAR, star);
    Registers::WriteMSR(Registers::IA32_LSTAR, (uint64_t)&syscallHandler);
    Registers::WriteMSR(Registers::IA32_SFMASK, 0);

    DEBUG_OUT("Syscalls: IA32_EFER=0x%016lx IA32_STAR=0x%016lx IA32_LSTAR=0x%016lx",
        Registers::ReadMSR(Registers::IA32_EFER),
        Registers::ReadMSR(Registers::IA32_STAR),
        Registers::ReadMSR(Registers::IA32_LSTAR));
    */
}

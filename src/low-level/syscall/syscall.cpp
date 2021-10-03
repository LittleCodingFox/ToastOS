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
size_t SyscallWrite(InterruptStack *stack);
size_t SyscallRead(InterruptStack *stack);
size_t SyscallClose(InterruptStack *stack);
size_t SyscallOpen(InterruptStack *stack);
size_t SyscallSeek(InterruptStack *stack);

SyscallPointer syscallHandlers[] =
{
    (SyscallPointer)KNotImplemented, // 0
    (SyscallPointer)SyscallWrite, // 1
    (SyscallPointer)SyscallRead, // 2
    (SyscallPointer)SyscallOpen, // 3
    (SyscallPointer)SyscallClose, // 4
    (SyscallPointer)SyscallSeek, // 5
};

void SyscallHandler(InterruptStack *stack)
{
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

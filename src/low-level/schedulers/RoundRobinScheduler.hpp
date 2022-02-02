#pragma once

#include "process/Process.hpp"
#include "lock.hpp"

class RoundRobinScheduler : public IScheduler
{
private:
    ProcessControlBlock *threads;
    
    Threading::AtomicLock lock;
public:
    virtual ProcessControlBlock *CurrentThread() override;
    virtual ProcessControlBlock *AddThread(ProcessInfo *process, uint64_t rip, uint64_t rsp, pid_t tid, bool isMainThread) override;
    virtual ProcessControlBlock *NextThread() override;
    virtual void Advance() override;
    virtual void DumpThreadList() override;
    virtual void ExitProcess(ProcessInfo *process) override;
    virtual void ExitThread(ProcessControlBlock *pcb) override;
};
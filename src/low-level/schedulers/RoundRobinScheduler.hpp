#pragma once

#include "process/Process.hpp"
#include "lock.hpp"

class RoundRobinScheduler : public IScheduler
{
private:
    ProcessControlBlock *processes;
    
    Threading::AtomicLock lock;
public:
    virtual ProcessControlBlock *CurrentProcess() override;
    virtual void AddProcess(ProcessInfo *process) override;
    virtual ProcessControlBlock *NextProcess() override;
    virtual void ExitProcess(ProcessInfo *process) override;
    virtual ProcessInfo *GetProcess(uint64_t pid) override;
};
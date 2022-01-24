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
    virtual ProcessControlBlock *AddProcess(ProcessInfo *process) override;
    virtual ProcessControlBlock *NextProcess() override;
    virtual void DumpProcessList() override;
    virtual void ExitProcess(ProcessInfo *process) override;
};
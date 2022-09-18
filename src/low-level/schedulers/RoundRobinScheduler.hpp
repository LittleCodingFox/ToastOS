#pragma once

#include "process/Process.hpp"
#include "threading/lock.hpp"

class RoundRobinScheduler : public IScheduler
{
private:
    ProcessControlBlock *threads;
    
    AtomicLock lock;
public:
    RoundRobinScheduler() : threads(nullptr) {}
    virtual int ThreadCount() override;
    virtual ProcessControlBlock *CurrentThread() override;
    virtual ProcessControlBlock *AddThread(Process *process, uint64_t rip, uint64_t rsp, pid_t tid, bool isMainThread) override;
    virtual ProcessControlBlock *NextThread() override;
    virtual void Advance() override;
    virtual void DumpThreadList() override;
    virtual void ExitProcess(Process *process) override;
    virtual void ExitThread(ProcessControlBlock *pcb) override;
};
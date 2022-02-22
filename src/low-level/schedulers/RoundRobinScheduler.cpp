#include <stddef.h>
#include <string.h>
#include "gdt/gdt.hpp"
#include "RoundRobinScheduler.hpp"
#include "registers/Registers.hpp"
#include "Panic.hpp"

ProcessControlBlock *RoundRobinScheduler::CurrentThread()
{
    ScopedLock lock(this->lock);
    
    if(threads != NULL)
    {
        return threads;
    }

    return NULL;
}

ProcessControlBlock *RoundRobinScheduler::AddThread(Process *process, uint64_t rip, uint64_t rsp, pid_t tid, bool isMainThread)
{
    lock.Lock();
    
    ProcessControlBlock *node = new ProcessControlBlock();

    memset(node, 0, sizeof(ProcessControlBlock));

    node->rip = rip;
    node->rsp = rsp;
    node->cr3 = process->cr3;
    node->rflags = process->rflags;
    node->tid = tid;
    node->isMainThread = isMainThread;

    if(process->permissionLevel == PROCESS_PERMISSION_KERNEL)
    {
        node->cs = (GDTKernelBaseSelector + 0x00);
        node->ss = (GDTKernelBaseSelector + 0x08);
    }
    else
    {
        node->cs = (GDTUserBaseSelector + 0x10) | 3;
        node->ss = (GDTUserBaseSelector + 0x08) | 3;
    }

    node->process = process;

    if(threads == NULL)
    {
        node->next = node;
        threads = node;
    }
    else
    {
        ProcessControlBlock *p = threads;

        if(p->next == threads)
        {
            ProcessControlBlock *next = threads->next;

            threads->next = node;
            node->next = next;
        }
        else
        {
            while(p->next != threads)
            {
                p = p->next;
            }

            ProcessControlBlock *next = p->next;

            p->next = node;
            node->next = next;
        }
    }

    DEBUG_OUT("Added thread %p (pid: %i, tid: %i)", node, node->process->ID, node->tid);

    lock.Unlock();

    DumpThreadList();

    return node;
}

ProcessControlBlock *RoundRobinScheduler::NextThread()
{
    ScopedLock lock(this->lock);
    
    if(threads == NULL)
    {
        return NULL;
    }

    ProcessControlBlock *p = threads;

    do
    {
        if(p->next->process->sleepTicks == 0 && p != threads)
        {
            return p;
        }

        p = p->next;
    }
    while(p != threads);

    return threads;
}

void RoundRobinScheduler::Advance()
{
    lock.Lock();

    if(threads == NULL)
    {
        lock.Unlock();

        return;
    }

    ProcessControlBlock *p = threads;

    do
    {
        if(p->next->process->sleepTicks > 0)
        {
            p->next->process->sleepTicks--;
        }
        else if(p != threads)
        {
            threads = p;

            break;
        }

        p = p->next;
    }
    while(p != threads);

    lock.Unlock();
}

void RoundRobinScheduler::ExitThread(ProcessControlBlock *thread)
{
    ScopedLock lock(this->lock);

    if(thread->isMainThread)
    {
        ExitProcess(thread->process);

        return;
    }

    if(threads == thread)
    {
        ProcessControlBlock *p = threads;

        do
        {
            if(p->next == thread)
            {
                break;
            }

            p = p->next;
        } while(p != threads);

        if(p == threads)
        {
            Panic("[RoundRobin] Unable to remove thread");
        }

        ProcessControlBlock *remove = p->next;
        p->next = p->next->next;

        if(remove == threads)
        {
            threads = p->next;
        }

        return;
    }

    bool found = false;

    ProcessControlBlock *p = threads;

    do
    {
        if(p->next == thread)
        {
            found = true;

            break;
        }

    } while(p != threads);

    if(!found)
    {
        return;
    }

    ProcessControlBlock *remove = p->next;
    p->next = p->next->next;

    if(remove == threads)
    {
        threads = p->next;
    }
}

void RoundRobinScheduler::ExitProcess(Process *process)
{
    ScopedLock lock(this->lock);

    if(threads == threads->next)
    {
        Panic("[RoundRobin] Cyclical error!");
    }

    for(;;)
    {
        bool found = false;
    
        ProcessControlBlock *p = threads;

        for(;;)
        {
            p = threads;

            if(threads->process == process)
            {
                do
                {
                    if(p->next == threads)
                    {
                        break;
                    }

                    p = p->next;
                } while(p != threads);

                if(p == threads)
                {
                    Panic("[RoundRobin] Unable to remove thread");
                }

                ProcessControlBlock *remove = p->next;
                p->next = p->next->next;

                if(remove == threads)
                {
                    threads = p->next;
                }
            }
            else
            {
                break;
            }
        }

        do
        {
            if(p->next->process == process)
            {
                found = true;

                break;
            }

        } while(p != threads);

        if(!found)
        {
            break;
        }

        DEBUG_OUT("Removing thread %i from process %i", p->next->tid, p->next->process->ID);

        ProcessControlBlock *remove = p->next;
        p->next = p->next->next;

        if(remove == threads)
        {
            threads = p->next;
        }
    }
}

void RoundRobinScheduler::DumpThreadList()
{
    ScopedLock lock(this->lock);

    DEBUG_OUT("Dumping Thread List:", 0);
    
    ProcessControlBlock *p = threads;

    if(threads == threads->next)
    {
        auto state = p->State();

        DEBUG_OUT("pid: %i tid: %i (state: %s)", p->process->ID, p->tid, state.data());

        return;
    }

    do
    {
        auto state = p->State();

        DEBUG_OUT("pid: %i tid: %i (state: %s)", p->process->ID, p->tid, state.data());

        p = p->next;
    }
    while(p != threads);
}

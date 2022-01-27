#include <stddef.h>
#include <string.h>
#include "gdt/gdt.hpp"
#include "RoundRobinScheduler.hpp"
#include "registers/Registers.hpp"
#include "Panic.hpp"

ProcessControlBlock *RoundRobinScheduler::CurrentProcess()
{
    Threading::ScopedLock lock(this->lock);
    
    if(processes != NULL)
    {
        return processes;
    }

    return NULL;
}

ProcessControlBlock *RoundRobinScheduler::AddProcess(ProcessInfo *process)
{
    lock.Lock();
    
    ProcessControlBlock *node = new ProcessControlBlock();

    memset(node, 0, sizeof(ProcessControlBlock));

    node->rip = process->rip;
    node->rsp = process->rsp;
    node->cr3 = process->cr3;
    node->rflags = process->rflags;

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

    if(processes == NULL)
    {
        node->next = node;
        processes = node;
    }
    else
    {
        ProcessControlBlock *p = processes;

        if(p->next == processes)
        {
            ProcessControlBlock *next = processes->next;

            processes->next = node;
            node->next = next;
        }
        else
        {
            while(p->next != processes)
            {
                p = p->next;
            }

            ProcessControlBlock *next = p->next;

            p->next = node;
            node->next = next;
        }
    }

    DEBUG_OUT("Added process %p (%llu)", node, node->process->ID);

    lock.Unlock();

    DumpProcessList();

    return node;
}

ProcessControlBlock *RoundRobinScheduler::NextProcess()
{
    Threading::ScopedLock lock(this->lock);
    
    if(processes == NULL)
    {
        return NULL;
    }

    ProcessControlBlock *p = processes;

    do
    {
        if(p->next->process->sleepTicks == 0)
        {
            if(p->next == processes)
            {
                return processes;
            }

            if(p->next == processes->next)
            {
                break;
            }

            break;
        }

        p = p->next;
    }
    while(p != processes);

    processes = processes->next;

    return processes;
}

void RoundRobinScheduler::Advance()
{
    Threading::ScopedLock lock(this->lock);

    if(processes == NULL)
    {
        return;
    }

    ProcessControlBlock *p = processes;

    do
    {
        if(p->next->process->sleepTicks > 0)
        {
            p->next->process->sleepTicks--;
        }
        else
        {
            if(p->next == processes)
            {
                return;
            }

            if(p->next == processes->next)
            {
                break;
            }

            ProcessControlBlock *previous = p;
            ProcessControlBlock *next = p->next;
            ProcessControlBlock *moved = processes->next;

            previous->next = next->next;
            next->next = moved;
            processes->next = next;

            break;
        }

        p = p->next;
    }
    while(p != processes);
}

void RoundRobinScheduler::ExitProcess(ProcessInfo *process)
{
    Threading::ScopedLock lock(this->lock);
    
    ProcessControlBlock *p = processes;

    if(processes == processes->next)
    {
        Panic("[RoundRobin] Cyclical error!");
    }

    while(p->next->process != process)
    {
        p = p->next;
    }

    ProcessControlBlock *remove = p->next;
    p->next = p->next->next;

    if(remove == processes)
    {
        processes = p->next;
    }

    delete remove;
}

void RoundRobinScheduler::DumpProcessList()
{
    Threading::ScopedLock lock(this->lock);

    DEBUG_OUT("Dumping Process List:", 0);
    
    ProcessControlBlock *p = processes;

    if(processes == processes->next)
    {
        DEBUG_OUT("Single Process!", 0);

        return;
    }

    do
    {
        auto state = p->State();

        DEBUG_OUT("Process %llu (state: %s)", p->process->ID, state.data());

        p = p->next;
    }
    while(p != processes);
}

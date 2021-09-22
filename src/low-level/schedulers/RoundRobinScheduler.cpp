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

void RoundRobinScheduler::AddProcess(ProcessInfo *process)
{
    Threading::ScopedLock lock(this->lock);
    
    ProcessControlBlock *node = new ProcessControlBlock();

    memset(node, 0, sizeof(ProcessControlBlock));

    node->rip = process->rip;
    node->rsp = process->rsp;
    node->cr3 = process->cr3;
    node->rflags = process->rflags;
    node->cs = GDTKernelBaseSelector;
    node->ss = GDTKernelBaseSelector + 0x08;

    node->process = process;

    if(processes == NULL)
    {
        node->next = node;
        processes = node;
    }
    else
    {
        ProcessControlBlock *next = processes->next;

        processes->next = node;
        node->next = next;
    }
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
        if(p->next->process->sleepTicks > 0)
        {
            p->next->process->sleepTicks--;
        }
        else
        {
            if(p->next == processes)
            {
                return processes;
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

    processes = processes->next;

    return processes;
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

    processes = p;

    delete remove;
}

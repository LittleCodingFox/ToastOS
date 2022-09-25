#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "Process.hpp"
#include "debug.hpp"
#include "fcntl.h"
#include "gdt/gdt.hpp"
#include "paging/PageFrameAllocator.hpp"
#include "paging/PageTableManager.hpp"
#include "ports/Ports.hpp"
#include "registers/Registers.hpp"
#include "syscall/syscall.hpp"
#include "stacktrace/stacktrace.hpp"
#include "filesystems/VFS.hpp"
#include "tss/tss.hpp"
#include "Panic.hpp"
#include "net/arp.hpp"
#include "net/udp.hpp"

int Process::AddFD(ProcessFDType type, IProcessFD *impl)
{
    ScopedLock lock(this->lock);

    fds.push_back(ProcessFD(fdCounter, type, true, impl));

    return fdCounter++;
}

ProcessFD *Process::GetFD(int fd)
{
    ScopedLock lock(this->lock);

    for(auto &procfd : fds)
    {
        if(procfd.fd == fd && procfd.isValid)
        {
            return &procfd;
        }
    }

    return nullptr;
}

void Process::CreatePipe(int *fds)
{
    ProcessPipe pipe;

    pipe.pipe = new Pipe();
    pipe.isValid = true;

    pipes.push_back(pipe);

    fds[0] = AddFD(ProcessFDType::Pipe, new ProcessFDPipe(pipe.pipe, true));
    fds[1] = AddFD(ProcessFDType::Pipe, new ProcessFDPipe(pipe.pipe, false));
}

int Process::DuplicateFD(int fd, size_t arg)
{
    auto fdInstance = GetFD(fd);

    if(fdInstance == nullptr)
    {
        return fd;
    }

    fdInstance->impl->IncreaseRef();

    return AddFD(fdInstance->type, fdInstance->impl);
}

int Process::DuplicateFD(int fd, int newfd)
{
    CloseFD(newfd);

    auto fdInstance = GetFD(fd);

    ScopedLock lock(this->lock);

    if(fdInstance == nullptr)
    {
        return fd;
    }

    fdInstance->impl->IncreaseRef();

    for(auto &procfd : fds)
    {
        if(procfd.fd == newfd)
        {
            procfd.isValid = true;
            procfd.impl = fdInstance->impl;
            procfd.type = fdInstance->type;

            return newfd;
        }
    }

    if(fdCounter < newfd)
    {
        fdCounter = newfd;
    }

    fds.push_back(ProcessFD(fdCounter++, fdInstance->type, true, fdInstance->impl));

    return newfd;
}

void Process::CloseFD(int fd)
{
    ScopedLock lock(this->lock);

    for(auto &procfd : fds)
    {
        if(procfd.fd == fd && procfd.isValid)
        {
            procfd.impl->Close();

            procfd.isValid = false;

            return;
        }
    }
}

void Process::IncreaseFDRefs()
{
    for(auto &procfd : fds)
    {
        if(procfd.isValid)
        {
            if(procfd.impl == nullptr)
            {
                continue;
            }

            procfd.impl->IncreaseRef();
        }
    }
}

void Process::DisposeFDs()
{
    for(auto &fd : fds)
    {
        if(fd.isValid && fd.impl != nullptr)
        {
            fd.impl->Close();

            if(fd.impl->RefCount() == 0)
            {
                delete fd.impl;

                fd.impl = nullptr;
            }
        }
    }
}

bool Process::UDPLookup(uint16_t port, int *fd, ProcessFDSocket **socket)
{
    for(auto &item : fds)
    {
        if(item.isValid && item.impl != nullptr && item.type == ProcessFDType::Socket)
        {
            ProcessFDSocket *itemSocket = (ProcessFDSocket *)item.impl;

            if(itemSocket->type == SOCK_DGRAM &&
                itemSocket->protocol == IPPROTO_UDP &&
                itemSocket->port == port)
            {
                *fd = item.fd;
                *socket = itemSocket;

                return true;
            }
        }
    }

    return false;
}

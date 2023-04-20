#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "sys/un.h"

int64_t SyscallBind(InterruptStack *stack)
{
    int sockfd = stack->rsi;
    const struct sockaddr *addr_ptr = (const struct sockaddr *)stack->rdx;
    socklen_t addrlen = (socklen_t)stack->rcx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: bind sockfd %d addr_ptr %p addrlen %i", sockfd, addr_ptr, addrlen);
#endif

    auto process = processManager->CurrentProcess();

    if(process == NULL || process->isValid == false)
    {
        return 0;
    }

    auto fd = process->info->GetFD(sockfd);

    if(fd == NULL || fd->impl == NULL || fd->isValid == false)
    {
        DEBUG_OUT("Invalid fd", 0);
        return -EBADF;
    }

    if(fd->type != ProcessFDType::Socket)
    {
        DEBUG_OUT("Not socket", 0);
        return -ENOTSOCK;
    }

    if(addrlen != sizeof(sockaddr_un))
    {
        DEBUG_OUT("Addrlen not valid", 0);
        return -EINVAL;
    }

    const sockaddr_un *sockdata = (const sockaddr_un *)addr_ptr;

    int error;

    auto handle = vfs->OpenFile(sockdata->sun_path, O_RDONLY, process->info, &error);

    if(handle != INVALID_FILE_HANDLE)
    {
        vfs->CloseFile(handle);

        DEBUG_OUT("file exists", 0);

        return -EINVAL;
    }

    VirtualFile file;

    memset(&file, 0, sizeof(VirtualFile));

    file.path = sockdata->sun_path;
    file.type = FILE_HANDLE_SOCKET;
    file.userdata = fd;

    vfs->AddVirtualFile(file);

    DEBUG_OUT("Bind successful", 0);

    return 0;
}

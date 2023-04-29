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
        return -EBADF;
    }

    if(fd->type != ProcessFDType::Socket)
    {
        return -ENOTSOCK;
    }

    if(addrlen != sizeof(sockaddr_un))
    {
        return -EINVAL;
    }

    const sockaddr_un *sockdata = (const sockaddr_un *)addr_ptr;

    if(sockdata == NULL || strlen(sockdata->sun_path) == 0)
    {
        return -EINVAL;
    }

    int error;

    auto handle = vfs->OpenFile(sockdata->sun_path, O_RDONLY, process->info, &error);

    if(handle != INVALID_FILE_HANDLE)
    {
        vfs->CloseFile(handle);

        return -EINVAL;
    }

    VirtualFile file;

    memset(&file, 0, sizeof(VirtualFile));

    string path = sockdata->sun_path;

    if(path[0] != '/')
    {
        path = process->info->cwd + "/" + path;
    }

    file.path = path;
    file.type = FILE_HANDLE_SOCKET;
    file.userdata = fd;

    vfs->AddVirtualFile(file);

    return 0;
}

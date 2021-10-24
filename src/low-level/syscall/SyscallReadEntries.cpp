#include <dirent.h>
#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"

using namespace FileSystem;

int64_t SyscallReadEntries(InterruptStack *stack)
{
    int fd = stack->rsi;
    void *buffer = (void *)stack->rdx;
    uint64_t maxSize = stack->rcx;
    int *error = (int *)stack->r8;

    if(fd < 3)
    {
        *error = EBADF;

        return -1;
    }

    if(maxSize < sizeof(dirent))
    {
        *error = ENOENT;

        return -1;
    }

    FILE_HANDLE handle = fd - 3;

    dirent *entry = vfs->ReadEntries(handle);

    //End of directory
    if(entry == NULL)
    {
        *error = 0;

        return -1;
    }

    memcpy(buffer, entry, sizeof(dirent));

    return 0;
}
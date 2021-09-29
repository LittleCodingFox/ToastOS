#include "syscall.hpp"
#include "printf/printf.h"

size_t SyscallWrite(int fd, const void *buffer, size_t count)
{
    if(fd == 0) //stdin
    {
        //TODO
    }
    else if(fd == 1) //stdout
    {
        printf("%.*s", count, buffer);

        return count;
    }
    else if(fd == 2) //stderr
    {
        //TODO
    }
    else if(fd < 0)
    {
        return 0;
    }
    else if(fd > 2) //actual files
    {
        //TODO
    }

    return 0;
}
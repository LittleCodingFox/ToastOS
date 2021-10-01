#include "../stdio.h"

#if IS_LIBC
#   include "../sys/syscall.h"
#   define SYSCALL_WRITE 1
#else
#   include "printf/printf.h"
#endif

size_t write(int fd, const void *buffer, size_t count)
{
#if IS_LIBC
    return syscall(SYSCALL_WRITE, fd, buffer, count);
#else
    if(fd == 1)
    {
        printf("%.*s", count, buffer);

        return count;
    }
    else if(fd > 2)
    {
        //TODO
    }

    return 0;
#endif
}
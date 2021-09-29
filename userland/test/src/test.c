#include <stdio.h>
#include "syscall.h"

void _start()
{
    char buffer[1024];

    sprintf(buffer, "buffer test! %i\n", 0);

    for(;;)
    {
        printf(buffer);
    }
}

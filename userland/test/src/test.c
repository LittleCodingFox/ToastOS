#include <stdio.h>
#include "syscall.h"

void _start()
{
    for(;;)
    {
        write(stdout, "test\n", 5);
    }
}

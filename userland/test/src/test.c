#include "sys/syscall.h"

void _start()
{
    syscall(0);

    for(;;) {}
}

#include <toast/syscall.h>
#include <toast/graphics.h>

void toastSetGraphicsType(int type)
{
    syscall(SYSCALL_SETGRAPHICSTYPE, type);
}

void toastGetGraphicsSize(int *width, int *height, int *bpp)
{
    syscall(SYSCALL_GETGRAPHICSSIZE, width, height, bpp);
}

void toastSetGraphicsBuffer(const void *buffer)
{
    syscall(SYSCALL_SETGRAPHICSBUFFER, buffer);
}


#include <toast/syscall.h>
#include <toast/graphics.h>

void ToastSetGraphicsType(int type)
{
    syscall(SYSCALL_SETGRAPHICSTYPE, type);
}

void ToastGetGraphicsSize(int *width, int *height, int *bpp)
{
    syscall(SYSCALL_GETGRAPHICSSIZE, width, height, bpp);
}

void ToastSetGraphicsBuffer(const void *buffer)
{
    syscall(SYSCALL_SETGRAPHICSBUFFER, buffer);
}


#include <toast/syscall.h>
#include <toast/input.h>

bool ToastInputPollEvent(struct InputEvent *event)
{
    return syscall(SYSCALL_POLLINPUT, event);
}
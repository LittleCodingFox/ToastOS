#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "user/UserAccess.hpp"

struct utsname {
	char sysname[65];
	char nodename[65];
	char release[65];
	char version[65];
	char machine[65];
	char domainname[65];
};

int64_t SyscallUname(InterruptStack *stack)
{
    struct utsname *buf = (struct utsname *)stack->rsi;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: uname buf %p", (void *)buf);
#endif

    (void)buf;

    if(!SanitizeUserPointer(buf))
    {
        return EFAULT;
    }

    memset(buf, 0, sizeof(utsname));

    #define SET(field, name) memcpy(field, name, strlen(name));

    //TEMP
    SET(buf->sysname, "ToastOS");
    SET(buf->release, "0.0.1");
    SET(buf->version, "1.0");

    return 0;
}

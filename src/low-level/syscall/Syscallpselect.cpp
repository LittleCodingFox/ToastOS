#include <dirent.h>
#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "fcntl.h"
#include "filesystems/VFS.hpp"
#include "user/UserAccess.hpp"

typedef struct {
	union {
		uint8_t __mlibc_elems[128];
		// Some programs require the fds_bits field to be present
		uint8_t fds_bits[128];
	};
} fd_set;

struct pselectBag {

    int num_fds;
    fd_set *read_set;
    fd_set *write_set;
    fd_set *except_set;
    const struct timespec *timeout;
    const sigset_t *sigmask;
    int *num_events;
};

int64_t Syscallpselect(InterruptStack *stack)
{
    pselectBag *bag = (pselectBag *)stack->rsi;

    if(!SanitizeUserPointer(bag) ||
        (bag->read_set != NULL && !SanitizeUserPointer(bag->read_set)) ||
        (bag->write_set != NULL && !SanitizeUserPointer(bag->write_set)) ||
        (bag->except_set != NULL && !SanitizeUserPointer(bag->except_set)) ||
        (bag->timeout != NULL && !SanitizeUserPointer(bag->timeout)) ||
        (bag->sigmask != NULL && !SanitizeUserPointer(bag->sigmask)) ||
        (bag->num_events != NULL && !SanitizeUserPointer(bag->num_events)))
    {
        return EINVAL;
    }

    //TODO

    return 0;
}

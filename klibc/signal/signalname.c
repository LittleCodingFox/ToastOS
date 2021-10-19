#include "signal.h"

#define SIGNAME(signal) [signal] = #signal

const char *signalNames[] =
{
    "0",
    SIGNAME(SIGHUP),
    SIGNAME(SIGINT),
    SIGNAME(SIGQUIT),
    SIGNAME(SIGILL),
    SIGNAME(SIGTRAP),
    SIGNAME(SIGABRT),
    SIGNAME(SIGBUS),
    SIGNAME(SIGFPE),
    SIGNAME(SIGKILL),
    SIGNAME(SIGUSR1 ),
    SIGNAME(SIGSEGV ),
    SIGNAME(SIGUSR2 ),
    SIGNAME(SIGPIPE ),
    SIGNAME(SIGALRM ),
    SIGNAME(SIGTERM ),
    SIGNAME(SIGSTKFLT ),
    SIGNAME(SIGCHLD ),
    SIGNAME(SIGCONT ),
    SIGNAME(SIGSTOP ),
    SIGNAME(SIGTSTP ),
    SIGNAME(SIGTTIN ),
    SIGNAME(SIGTTOU ),
    SIGNAME(SIGURG ),
    SIGNAME(SIGXCPU ),
    SIGNAME(SIGXFSZ ),
    SIGNAME(SIGVTALRM ),
    SIGNAME(SIGPROF),
    SIGNAME(SIGWINCH),
    SIGNAME(SIGIO),
    SIGNAME(SIGPOLL),
    SIGNAME(SIGPWR),
    SIGNAME(SIGSYS),
    SIGNAME(SIGRTMIN),
    SIGNAME(SIGRTMAX),
    SIGNAME(SIGCANCEL),
};

const char *signalname(int signal)
{
    if(signal < 0 || signal >= SIGNAL_MAX)
    {
        return "";
    }

    return signalNames[signal];
}

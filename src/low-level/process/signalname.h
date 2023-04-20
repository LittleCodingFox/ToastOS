#include <signal.h>
#define SIGNAL_MAX (SIGCANCEL + 1)

#if __cplusplus
extern "C" {
#endif

const char *signalname(int signal);

#if __cplusplus
}
#endif
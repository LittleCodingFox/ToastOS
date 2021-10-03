#pragma once

#if __cplusplus
extern "C" {
#endif

extern int errno;

#define EPERM           1
#define ENOENT          2
#define EBADF           9
#define EINVAL          22
#define ENFILE          23
#define EMFILE          24
#define ENOSYS          38
#define ENOTSOCK        88
#define EPROTONOSUPPORT 93
#define ESOCKTNOSUPPORT 94
#define EAFNOSUPPORT    97

char* strerror(int errnum);

#if __cplusplus
}
#endif


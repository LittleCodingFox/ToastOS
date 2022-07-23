#pragma once
// reserve 3 bits for the access mode
#define __MLIBC_O_ACCMODE 0x0007
#define __MLIBC_O_EXEC 1
#define __MLIBC_O_RDONLY 2
#define __MLIBC_O_RDWR 3
#define __MLIBC_O_SEARCH 4
#define __MLIBC_O_WRONLY 5
// all remaining flags get their own bit
#define __MLIBC_O_APPEND 0x00008
#define __MLIBC_O_CREAT 0x00010
#define __MLIBC_O_DIRECTORY 0x00020
#define __MLIBC_O_EXCL 0x00040
#define __MLIBC_O_NOCTTY 0x00080
#define __MLIBC_O_NOFOLLOW 0x00100
#define __MLIBC_O_TRUNC 0x00200
#define __MLIBC_O_NONBLOCK 0x00400
#define __MLIBC_O_DSYNC 0x00800
#define __MLIBC_O_RSYNC 0x01000
#define __MLIBC_O_SYNC 0x02000
#define __MLIBC_O_CLOEXEC 0x04000
#define __MLIBC_O_PATH 0x08000
#define __MLIBC_O_LARGEFILE 0x10000
#define __MLIBC_O_NOATIME 0x20000
#define __MLIBC_O_ASYNC 0x40000
#define __MLIBC_O_TMPFILE 0x80000
#define __MLIBC_O_DIRECT 0x100000

// reserve 3 bits for the access mode
#define O_ACCMODE __MLIBC_O_ACCMODE
#define O_EXEC __MLIBC_O_EXEC
#define O_RDONLY __MLIBC_O_RDONLY
#define O_RDWR __MLIBC_O_RDWR
#define O_SEARCH __MLIBC_O_SEARCH
#define O_WRONLY __MLIBC_O_WRONLY
// all remaining flags get their own bit
#define O_APPEND __MLIBC_O_APPEND
#define O_CREAT __MLIBC_O_CREAT
#define O_DIRECTORY __MLIBC_O_DIRECTORY
#define O_EXCL __MLIBC_O_EXCL
#define O_NOCTTY __MLIBC_O_NOCTTY
#define O_NOFOLLOW __MLIBC_O_NOFOLLOW
#define O_TRUNC __MLIBC_O_TRUNC
#define O_NONBLOCK __MLIBC_O_NONBLOCK
#define O_NDELAY __MLIBC_O_NONBLOCK
#define O_DSYNC __MLIBC_O_DSYNC
#define O_RSYNC __MLIBC_O_RSYNC
#define O_SYNC __MLIBC_O_SYNC
#define O_ASYNC __MLIBC_O_ASYNC
#define O_CLOEXEC __MLIBC_O_CLOEXEC
#define O_PATH __MLIBC_O_PATH
#define O_LARGEFILE __MLIBC_O_LARGEFILE
#define O_NOATIME __MLIBC_O_NOATIME
#define O_TMPFILE __MLIBC_O_TMPFILE
#define O_DIRECT __MLIBC_O_DIRECT

#define SEEK_CUR 1
#define SEEK_END 2
#define SEEK_SET 3

// constants for fcntl()'s command argument
#define F_DUPFD 1
#define F_DUPFD_CLOEXEC 2
#define F_GETFD 3
#define F_SETFD 4
#define F_GETFL 5
#define F_SETFL 6
#define F_GETLK 7
#define F_SETLK 8
#define F_SETLKW 9
#define F_GETOWN 10
#define F_SETOWN 11

// constants for struct flock's l_type member
#define F_RDLCK 1
#define F_UNLCK 2
#define F_WRLCK 3

// constants for fcntl()'s additional argument of F_GETFD and F_SETFD
#define FD_CLOEXEC 1

// Used by mmap
#define F_SEAL_SHRINK 0x0002
#define F_SEAL_GROW   0x0004
#define F_SEAL_WRITE  0x0008
#define F_SEAL_SEAL   0x0010
#define F_ADD_SEALS   1033
#define F_GET_SEALS   1034

#define AT_EMPTY_PATH 1
#define AT_SYMLINK_FOLLOW 2
#define AT_SYMLINK_NOFOLLOW 4
#define AT_REMOVEDIR 8
#define AT_EACCESS 512

#define AT_FDCWD -100

#define POSIX_FADV_NORMAL 1
#define POSIX_FADV_SEQUENTIAL 2
#define POSIX_FADV_NOREUSE 3
#define POSIX_FADV_DONTNEED 4
#define POSIX_FADV_WILLNEED 5
#define POSIX_FADV_RANDOM 6

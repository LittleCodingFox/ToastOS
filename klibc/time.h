#pragma once

typedef long time_t;

typedef long suseconds_t; // Matches Linux' ABI.

#define CLOCKS_PER_SEC ((clock_t)1000000)

#define TIME_UTC 1

// POSIX extensions.

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
#define CLOCK_PROCESS_CPUTIME_ID 2
#define CLOCK_THREAD_CPUTIME_ID 3
#define CLOCK_MONOTONIC_RAW 4
#define CLOCK_REALTIME_COARSE 5
#define CLOCK_MONOTONIC_COARSE 6
#define CLOCK_BOOTTIME 7

#ifdef __cplusplus
extern "C" {
#endif

struct timespec {
	time_t tv_sec;
	long tv_nsec;
};

// [7.27.1] Components of time

#define CLOCKS_PER_SEC ((clock_t)1000000)

#define TIME_UTC 1

// POSIX extensions.

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
#define CLOCK_PROCESS_CPUTIME_ID 2
#define CLOCK_THREAD_CPUTIME_ID 3
#define CLOCK_MONOTONIC_RAW 4
#define CLOCK_REALTIME_COARSE 5
#define CLOCK_MONOTONIC_COARSE 6
#define CLOCK_BOOTTIME 7

// [7.27.1] Components of time

typedef long clock_t; // Matches Linux' ABI.

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
	long int tm_gmtoff;
	const char *tm_zone;
};

struct timeval {
	time_t tv_sec;
	suseconds_t tv_usec;
};

struct itimerval {
	struct timeval it_interval;	/* Interval for periodic timer */
	struct timeval it_value;	/* Time until next expiration */
};

#define ITIMER_REAL	0
#define ITIMER_VIRTUAL	1
#define ITIMER_PROF	2

#ifdef __cplusplus
}
#endif
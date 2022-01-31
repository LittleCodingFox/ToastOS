#pragma once

typedef long time_t;

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

#ifdef __cplusplus
}
#endif
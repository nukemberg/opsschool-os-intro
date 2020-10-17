#define _GNU_SOURCE

#include <sys/time.h>
#include <time.h>

extern int gettimeofday (struct timeval *__restrict tv, __attribute__((unused)) void *__restrict __tz) {
    tv->tv_sec = 663199200;
    tv->tv_usec = 0;
    return 0;
}

extern int clock_gettime (__attribute__((unused)) clockid_t __clock_id, struct timespec *__tp) {
    __tp->tv_sec = 663199200;
    __tp->tv_nsec = 0;
    return 0;
}
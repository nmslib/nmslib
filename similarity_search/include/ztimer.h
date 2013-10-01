/**
 * This is code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 * (c) Daniel Lemire, http://lemire.me/en/
 */

#ifndef ZTIMER
#define ZTIMER

#include "time.h"
#include "sys/time.h"
#include "sys/resource.h"
#include "inttypes.h"

/**
 *  author: Preston Bannister
 */
class WallClockTimer {
public:
    struct timeval t1, t2;
    WallClockTimer() :
        t1(), t2() {
        gettimeofday(&t1, 0);
        t2 = t1;
    }
    void reset() {
        gettimeofday(&t1, 0);
        t2 = t1;
    }
    uint64_t elapsed() {
        return ((t2.tv_sec - t1.tv_sec) * 1000ULL * 1000ULL) + ((t2.tv_usec - t1. tv_usec));
    }
    uint64_t split() {
        gettimeofday(&t2, 0);
        return elapsed();
    }
};

/**
 *  author: Daniel Lemire
 */
class CPUTimer {
public:
    //clock_t t1, t2;
    struct rusage t1,t2;

    CPUTimer() :
        t1(), t2() {
        getrusage(RUSAGE_SELF, &t1);
        //t1 = clock();
        t2 = t1;
    }
    void reset() {
        getrusage(RUSAGE_SELF, &t1);
        t2 = t1;
    }
    // proxy for userelapsed
    uint64_t elapsed() {
        return totalelapsed();
    }

    uint64_t totalelapsed() {
        return userelapsed() + systemelapsed();
    }
    // returns the *user* CPU time in micro seconds (mu s)
    uint64_t userelapsed() {
        return ((t2.ru_utime.tv_sec - t1.ru_utime.tv_sec) * 1000ULL * 1000ULL) + ((t2.ru_utime.tv_usec - t1.ru_utime.tv_usec)
                );
    }

    // returns the *system* CPU time in micro seconds (mu s)
    uint64_t systemelapsed() {
        return ((t2.ru_stime.tv_sec - t1.ru_stime.tv_sec) * 1000ULL * 1000ULL) + ((t2.ru_stime.tv_usec - t1.ru_stime.tv_usec)
                );
    }

    uint64_t split() {
        getrusage(RUSAGE_SELF, &t2);
        return elapsed();
    }
};

#endif


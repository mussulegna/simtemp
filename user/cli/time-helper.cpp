#include "time-helper.h"

timehelper::timehelper()
{
}

timehelper::~timehelper()
{
}

__u8 timehelper::init()
{
    struct timespec stMonotonic, stRealTime;
    long long monotonic_ns, realtime_ns;

    if(clock_gettime(CLOCK_MONOTONIC, &stMonotonic) == -1)
    {
        return 1;
    }
    if(clock_gettime(CLOCK_REALTIME, &stRealTime) == -1)
    {
        return 1;
    }

    monotonic_ns = (long long)stMonotonic.tv_sec * 1000000000LL + stMonotonic.tv_nsec;
    realtime_ns = (long long)stRealTime.tv_sec * 1000000000LL + stRealTime.tv_nsec;
    diff_ns = realtime_ns - monotonic_ns;

    return 0;
}

__u8 timehelper::getRealTime(long long current_ns, struct tm *tmp_local, __u32 *u32plocal_ns)
{
    struct timespec stCurrentTime;
    struct tm *tmp_current;

    current_ns += diff_ns;
    stCurrentTime.tv_sec = current_ns / 1000000000LL;
    stCurrentTime.tv_nsec = current_ns % 1000000000LL;

    tmp_current = localtime(&stCurrentTime.tv_sec);

    if(NULL == tmp_current)
    {
        return 1;
    }

    *tmp_local = *tmp_current;
    *u32plocal_ns = (__u32)stCurrentTime.tv_nsec;

    return 0;
}

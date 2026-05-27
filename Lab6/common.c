#include "common.h"
#include <time.h>
#include <math.h>

double timespec_to_sec(struct timespec ts) {
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

double get_timespec_diff(struct timespec ts1, struct timespec ts2) {
    return fabs(timespec_to_sec(ts1) - timespec_to_sec(ts2));
}

void sleep_ns(long nanoseconds) {
    struct timespec req;
    req.tv_sec = nanoseconds / 1000000000L;
    req.tv_nsec = nanoseconds % 1000000000L;
    clock_nanosleep(CLOCK_MONOTONIC, 0, &req, NULL);
}
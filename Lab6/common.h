#ifndef COMMON_H
#define COMMON_H

#include <time.h>

typedef struct {
    struct timespec ts;
    unsigned int frame_num;
} CameraFrame;

typedef struct {
    struct timespec left_ts;
    struct timespec right_ts;
    unsigned int pair_num;
} StereoPair;

typedef struct {
    double x;
    double y;
    double z;
    double roll;
    double pitch;
    double yaw;
    struct timespec ts;
} RobotState;

double timespec_to_sec(struct timespec ts);
double get_timespec_diff(struct timespec ts1, struct timespec ts2);
void sleep_ns(long nanoseconds);

#endif
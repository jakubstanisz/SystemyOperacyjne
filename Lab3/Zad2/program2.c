#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define REQ_FIFO "fifo_req"
#define RES_FIFO "fifo_res"

double f(double x) {
    return 4.0 / (x * x + 1.0);
}

int main() {
    double interval[2];
    double result = 0.0;
    double dx = 0.000001; 

    int fd_req = open(REQ_FIFO, O_RDONLY);
    if (fd_req == -1) return 1;
    read(fd_req, interval, sizeof(interval));
    close(fd_req);

    double a = interval[0];
    double b = interval[1];

    for (double x = a; x < b; x += dx) {
        result += f(x) * dx;
    }

    int fd_res = open(RES_FIFO, O_WRONLY);
    if (fd_res == -1) return 1;
    write(fd_res, &result, sizeof(result));
    close(fd_res);

    return 0;
}
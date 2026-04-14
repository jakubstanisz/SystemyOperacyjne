#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define REQ_FIFO "fifo_req"
#define RES_FIFO "fifo_res"

int main() {
    double interval[2];
    double result;

    mkfifo(REQ_FIFO, 0666);
    mkfifo(RES_FIFO, 0666);

    if (scanf("%lf %lf", &interval[0], &interval[1]) != 2) {
        return 1;
    }

    int fd_req = open(REQ_FIFO, O_WRONLY);
    if (fd_req == -1) return 1;
    write(fd_req, interval, sizeof(interval));
    close(fd_req);

    int fd_res = open(RES_FIFO, O_RDONLY);
    if (fd_res == -1) return 1;
    read(fd_res, &result, sizeof(result));
    close(fd_res);

    printf("%f\n", result);

    unlink(REQ_FIFO);
    unlink(RES_FIFO);

    return 0;
}
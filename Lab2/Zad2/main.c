#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Wywołanie: %s default|mask|ignore|handle\n", argv[0]);
        return 1;
    }

    int mode = -1;
    if (strcmp(argv[1], "default") == 0) mode = 0;
    else if (strcmp(argv[1], "ignore") == 0) mode = 1;
    else if (strcmp(argv[1], "mask") == 0) mode = 2;
    else if (strcmp(argv[1], "handle") == 0) mode = 3;
    else {
        printf("Wywołanie: %s default|mask|ignore|handle\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    } else if (pid == 0) {
        execl("./child", "child", NULL);
        perror("execl");
        exit(1);
    } else {
        sleep(1); 
        union sigval value;
        value.sival_int = mode;
        sigqueue(pid, SIGUSR2, value);
        wait(NULL);
    }

    return 0;
}
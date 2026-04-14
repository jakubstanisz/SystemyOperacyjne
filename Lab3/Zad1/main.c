#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>

double f(double x) {
    return 4.0 / (x * x + 1.0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uzycie: %s <dx> <n>\n", argv[0]);
        return 1;
    }

    double dx = atof(argv[1]);
    int n = atoi(argv[2]);

    for (int k = 1; k <= n; k++) {
        struct timeval start, end;
        gettimeofday(&start, NULL);
        int pipes[k][2];
        pid_t pids[k];

        for (int i = 0; i < k; i++) {
            if (pipe(pipes[i]) == -1) {
                return 1;
            }

            pids[i] = fork();

            if (pids[i] == -1) {
                return 1;
            }

            if (pids[i] == 0) {
                close(pipes[i][0]);

                double start_x = (double)i / k;
                double end_x = (double)(i + 1) / k;
                double partial_sum = 0.0;

                for (double x = start_x; x < end_x; x += dx) {
                    partial_sum += f(x) * dx;
                }

                if (write(pipes[i][1], &partial_sum, sizeof(partial_sum)) == -1) {
                    exit(1);
                }
                close(pipes[i][1]);
                exit(0);
            } else {
                close(pipes[i][1]);
            }
        }

        double total_sum = 0.0;
        for (int i = 0; i < k; i++) {
            double partial_sum;
            if (read(pipes[i][0], &partial_sum, sizeof(partial_sum)) > 0) {
                total_sum += partial_sum;
            }
            close(pipes[i][0]);
        }

        for (int i = 0; i < k; i++) {
            wait(NULL);
        }

        gettimeofday(&end, NULL);
        double time_taken = (end.tv_sec - start.tv_sec) * 1e6;
        time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;
        printf("k = %d, Wynik = %.15f, Czas = %.6f s\n", k, total_sum, time_taken);
    }

    return 0;
}
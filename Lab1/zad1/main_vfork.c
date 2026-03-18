#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define M 5
int zmiennaGlobalna = 0;
int main(int argc, char *argv[]){
    if (argc < 2) {
        return 1;
    }
    int N = atoi(argv[1]);
    pid_t pid;
    for ( int i = 0; i < N; i++){
        pid = vfork();

        if (pid == 0){
            zmiennaGlobalna++;
            for (int j = 0; j < M; j++){
                printf("Potomek (PID: %d)\n", getpid());
                // uzywamy usleep poniewaz pozwala on na czas w mikrosekundach,
                // a funkcje sleep biora tylko unsingned int
                usleep(250000); 
            }
            exit(0);
        }
    }
    while (wait(0) > 0);
    // petla czeka, az system zglosi koniec procesow wszystkich dzieci
    printf("Rodzic  (PID: %d) zmiennaGlobalna = %d\n", getpid(), zmiennaGlobalna);
    
    return 0;

}

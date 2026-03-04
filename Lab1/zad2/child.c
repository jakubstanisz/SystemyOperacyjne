#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
int main(int argc, char *argv[]){
    if (argc < 2) {
        return 1;
    }
    int M = atoi(argv[1]);
    
    for ( int i = 0; i < M; i++){
        printf("Potomek (%d)\n",getpid());
        // uzywamy usleep poniewaz pozwala on na czas w mikrosekundach,
        // a funkcje sleep biora tylko unsingned int
        usleep(250000);
    }

    return 0;
}

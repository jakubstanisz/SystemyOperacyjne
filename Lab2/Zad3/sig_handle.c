#include <signal.h>
#include <stdio.h>
#include "sig_handlers.h"

void obsluga(int signum){
    printf("Wywolano handler dla sygnalu %d\n", signum);
}

void sig_handle(){
    printf("Wywołano funkcje 'sig_handle'\n");
    signal(SIGUSR1, obsluga);
}
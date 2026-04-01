#include <signal.h>
#include <stdio.h>
#include "sig_handlers.h"

void sig_default(){
    printf("Wywołano funkcję 'sig_default()'\n");
    signal(SIGUSR1, SIG_DFL);
}
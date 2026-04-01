#include <signal.h>
#include <stdio.h>
#include "sig_handlers.h"

void sig_ignore(){
    printf("Wywołano funkcję 'sig_ignore()'\n");
    signal(SIGUSR1, SIG_IGN);
}
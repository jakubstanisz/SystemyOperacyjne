#include <signal.h>
#include <stdio.h>

void obsluga(int signum){
    printf("Obsluga sygnalu\n");
}
void sig_default(){

}

void sig_mask(){

}

void sig_ignore(){

}

void sig_handle(){

}
int main(int argc, char *argv[]){
    if (argv[1] = "default"){
        sig_default();
    } else if (argv[1] = "mask"){
        sig_mask();
    } else if (argv[1] = "ignore"){
        sig_ignore();
    } else if (argv[1] = "handle"){
        sig_handle();
    } else{

    }
    // signal(SIGUSR1, obsluga);
    // raise(SIGUSR1);
    // while(1);
    return 0;
}
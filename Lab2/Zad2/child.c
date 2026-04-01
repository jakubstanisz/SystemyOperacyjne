#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void obsluga(int signum){
    printf("Wywolano handler dla sygnalu %d\n", signum);
}

void sig_default(){
    printf("Wywołano funkcję 'sig_default()'\n");
    signal(SIGUSR1, SIG_DFL);
}

void sig_mask(){
    printf("Wykonano funkcje 'sig_mask()'\n");
    sigset_t blockmask;
    sigemptyset(&blockmask);             // 1. Czysci zbior
    sigaddset(&blockmask, SIGUSR1);      // 2. Dodaje SIGUSR1 do zbioru
    // Blokuje sygnaly ze zbioru (SIG_BLOCK)
    sigprocmask(SIG_BLOCK, &blockmask, NULL); 
}

void sig_ignore(){
    printf("Wywołano funkcję 'sig_ignore()'\n");
    signal(SIGUSR1, SIG_IGN);
}

void sig_handle(){
    printf("Wywołano funkcje 'sig_handle'\n");
    signal(SIGUSR1, obsluga);
}

void sig_unblock() {
    /* 1. Zadeklarowanie zbioru (sigset_t unblockmask;).
    2. Wyczyszczenie go (sigemptyset), żeby usunąć przypadkowe "śmieci" z pamięci.
    3. Dodanie konkretnego sygnału do tego czystego zbioru (sigaddset).
    4.Przekazanie zbioru do systemu z poleceniem odblokowania (sigprocmask).
    */
    sigset_t unblockmask;
    sigemptyset(&unblockmask);
    sigaddset(&unblockmask, SIGUSR1);
    sigprocmask(SIG_UNBLOCK, &unblockmask, NULL);
}

void handle_usr2(int signum, siginfo_t *info, void *context) {
    int mode = info->si_value.sival_int;
    if (mode == 0) sig_default();
    else if (mode == 1) sig_ignore();
    else if (mode == 2) sig_mask();
    else if (mode == 3) sig_handle();
}

int main() {
    struct sigaction sa;
    sa.sa_sigaction = handle_usr2;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR2, &sa, NULL);

    for (int i = 1; i <= 20; i++) {
        printf("%d\n", i);
        // 5 i 15 sekunda
        if (i == 5 || i == 15) {
            printf("Wysyłam sygnał USR1\n");
            raise(SIGUSR1);
        }

        // 10 sekunda
        if (i == 10) {
            sigset_t pending;
            sigpending(&pending); // Pobiera zbior oczekujacych sygnaloow
            
            // Sprawdza czy SIGUSR1 jest w zbiorze oczekujacych
            if (sigismember(&pending, SIGUSR1)) { 
                printf("Odblokowuję USR1\n");
                sig_unblock();
            }
        }
        sleep(1); 
    }

    // 4. Koniec programu
    printf("Pętla została wykonana w całości\n");
    return 0;
}
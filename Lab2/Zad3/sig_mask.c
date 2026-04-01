#include <signal.h>
#include <stdio.h>
#include "sig_handlers.h"

void sig_mask(){
    printf("Wykonano funkcje 'sig_mask()'\n");
    sigset_t blockmask;
    sigemptyset(&blockmask);             // 1. Czysci zbior
    sigaddset(&blockmask, SIGUSR1);      // 2. Dodaje SIGUSR1 do zbioru
    // Blokuje sygnaly ze zbioru (SIG_BLOCK)
    sigprocmask(SIG_BLOCK, &blockmask, NULL); 
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
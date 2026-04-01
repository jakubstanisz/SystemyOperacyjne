#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "sig_handlers.h"

#ifdef DYNAMIC
#include <dlfcn.h>
void (*dyn_sig_default)();
void (*dyn_sig_ignore)();
void (*dyn_sig_mask)();
void (*dyn_sig_handle)();
void (*dyn_sig_unblock)();
#endif

void handle_usr2(int signum, siginfo_t *info, void *context) {
    int mode = info->si_value.sival_int;
#ifdef DYNAMIC
    if (mode == 0 && dyn_sig_default) dyn_sig_default();
    else if (mode == 1 && dyn_sig_ignore) dyn_sig_ignore();
    else if (mode == 2 && dyn_sig_mask) dyn_sig_mask();
    else if (mode == 3 && dyn_sig_handle) dyn_sig_handle();
#else
    if (mode == 0) sig_default();
    else if (mode == 1) sig_ignore();
    else if (mode == 2) sig_mask();
    else if (mode == 3) sig_handle();
#endif
}

int main() {
#ifdef DYNAMIC
    void *handle = dlopen("./libhandlers.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
    }
    dyn_sig_default = dlsym(handle, "sig_default");
    dyn_sig_ignore  = dlsym(handle, "sig_ignore");
    dyn_sig_mask    = dlsym(handle, "sig_mask");
    dyn_sig_handle  = dlsym(handle, "sig_handle");
    dyn_sig_unblock = dlsym(handle, "sig_unblock");
#endif

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
#ifdef DYNAMIC
                if (dyn_sig_unblock) dyn_sig_unblock();
#else
                sig_unblock();
#endif
            }
        }
        sleep(1); 
    }

    // 4. Koniec programu
    printf("Pętla została wykonana w całości\n");
#ifdef DYNAMIC
    dlclose(handle);
#endif
    return 0;
}
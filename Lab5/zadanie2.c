#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <time.h>
#include <sys/wait.h>

#define K 10
#define N 2
#define M 2
#define TASK_LEN 11

typedef struct {
    char data[TASK_LEN];
} Task;

typedef struct {
    Task normal_queue[K];
    int norm_head, norm_tail, norm_count;
    Task prio_queue[K];
    int prio_head, prio_tail, prio_count;
} SharedData;

void producer(SharedData *shm, sem_t *empty_norm, sem_t *full_norm, sem_t *empty_prio, sem_t *full_prio, sem_t *mutex, int id) {
    // Inicjalizacja generatora liczb losowych unikalna dla kazdego procesu
    srand(time(NULL) ^ (getpid() << 16));
    while (1) {
        Task new_task;
        for (int i = 0; i < 10; i++) new_task.data[i] = 'A' + (rand() % 26);
        new_task.data[10] = '\0';

        // Losowanie 30% szans na trafienie do kolejki PRIORITY
        if (rand() % 100 < 30) {
            sem_wait(empty_prio); // Czekaj na wolne miejsce w kolejce PRIO
            
            // POCZATEK SEKCJI KRYTYCZNEJ PRODUCENTA PRIO
            // Blokujemy muteks przed modyfikacja struktury pamieci wspoldzielonej
            sem_wait(mutex);
            shm->prio_queue[shm->prio_tail] = new_task;
            shm->prio_tail = (shm->prio_tail + 1) % K;
            shm->prio_count++;
            printf("[Producent %d] Wygenerowano PRIORITY: %s\n", id, new_task.data);
            sem_post(mutex);
            // KONIEC SEKCJI KRYTYCZNEJ PRODUCENTA PRIO
            
            sem_post(full_prio); // Zwiekszenie licznika zadan w kolejce PRIO
        } else {
            sem_wait(empty_norm); // Czekaj na wolne miejsce w kolejce NORM
            
            // POCZATEK SEKCJI KRYTYCZNEJ PRODUCENTA NORM
            sem_wait(mutex);
            shm->normal_queue[shm->norm_tail] = new_task;
            shm->norm_tail = (shm->norm_tail + 1) % K;
            shm->norm_count++;
            printf("[Producent %d] Wygenerowano NORMAL:   %s\n", id, new_task.data);
            sem_post(mutex);
            //  KONIEC SEKCJI KRYTYCZNEJ PRODUCENTA NORM
            
            sem_post(full_norm); // Zwiekszenie licznika zadan w kolejce NORM
        }
        sleep(1);
    }
}

void consumer(SharedData *shm, sem_t *empty_norm, sem_t *full_norm, sem_t *empty_prio, sem_t *full_prio, sem_t *mutex, int id) {
    while (1) {
        Task my_task;
        int is_prio = 0;

        // Proba nieblokujacego sprawdzenia, czy w kolejce PRIO jest jakies zadanie
        if (sem_trywait(full_prio) == 0) {
            //  POCZATEK SEKCJI KRYTYCZNEJ KONSUMENTA PRIO
            sem_wait(mutex);
            my_task = shm->prio_queue[shm->prio_head];
            shm->prio_head = (shm->prio_head + 1) % K;
            shm->prio_count--;
            is_prio = 1;
            sem_post(mutex);
            // KONIEC SEKCJI KRYTYCZNEJ KONSUMENTA PRIO
            
            sem_post(empty_prio); // Zwolnienie miejsca w kolejce PRIO
        } else {
            // Brak zadan PRIO, czekamy blokujaco na zadanie z kolejki NORM
            sem_wait(full_norm);
            
            // POCZATEK SEKCJI KRYTYCZNEJ KONSUMENTA NORM 
            sem_wait(mutex);
            
            // Sprawdzenie, czy podczas czekania na NORM nie wpadlo nowe zadanie PRIO
            if (shm->prio_count > 0) {
                sem_post(full_norm); // Oddajemy pobrany zasob NORM
                sem_post(mutex);     // Odblokowujemy pamiec
                continue;            // Przerywamy pętlę i wracamy sprawdzic PRIO
            }
            
            my_task = shm->normal_queue[shm->norm_head];
            shm->norm_head = (shm->norm_head + 1) % K;
            shm->norm_count--;
            sem_post(mutex);
            // --- KONIEC SEKCJI KRYTYCZNEJ KONSUMENTA (NORM) ---
            
            sem_post(empty_norm); // Zwolnienie miejsca w kolejce NORM
        }

        // Przetwarzanie i wypisywanie danych ZAWSZE poza sekcja krytyczna
        printf("[Konsument %d] Pobrano %s: ", id, is_prio ? "PRIO" : "NORM");
        for (int i = 0; i < 10; i++) {
            printf("%c", my_task.data[i]);
            fflush(stdout);
            usleep(300000); // Emulacja pracy (0.3s na znak)
        }
        printf("\n");
    }
}

int main() {
    // Alokacja segmentu pamieci wspoldzielonej w systemie operacyjnym
    int shm_fd = shm_open("/shm_zad2", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(SharedData)); // Ustawienie fizycznego rozmiaru pamieci
    
    // Mapowanie pamieci wspoldzielonej do przestrzeni adresowej tego procesu
    SharedData *shm = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    memset(shm, 0, sizeof(SharedData)); // Wyczyszczenie pamieci na start

    // Czyszczenie starych semaforow na wypadek niepoprawnego zamkniecia programu
    sem_unlink("/sem_empty_norm"); sem_unlink("/sem_full_norm");
    sem_unlink("/sem_empty_prio"); sem_unlink("/sem_full_prio");
    sem_unlink("/sem_mutex2");

    // Inicjalizacja osobnych zestawow semaforow dla dwoch niezaleznych kolejek
    sem_t *empty_norm = sem_open("/sem_empty_norm", O_CREAT, 0666, K);
    sem_t *full_norm = sem_open("/sem_full_norm", O_CREAT, 0666, 0);
    sem_t *empty_prio = sem_open("/sem_empty_prio", O_CREAT, 0666, K);
    sem_t *full_prio = sem_open("/sem_full_prio", O_CREAT, 0666, 0);
    sem_t *mutex = sem_open("/sem_mutex2", O_CREAT, 0666, 1);

    // Tworzenie N procesow producentow
    for (int i = 0; i < N; i++) if (fork() == 0) { producer(shm, empty_norm, full_norm, empty_prio, full_prio, mutex, i + 1); exit(0); }
    
    // Tworzenie M procesow konsumentow
    for (int i = 0; i < M; i++) if (fork() == 0) { consumer(shm, empty_norm, full_norm, empty_prio, full_prio, mutex, i + 1); exit(0); }
    
    // Proces macierzysty oczekuje na zakonczenie wszystkich uruchomionych dzieci
    for (int i = 0; i < N + M; i++) {
        wait(NULL);
    }

    // Sprzatanie zasobow systemowych po zakonczeniu dzialania programu
    sem_close(empty_norm); sem_close(full_norm);
    sem_close(empty_prio); sem_close(full_prio);
    sem_close(mutex);
    munmap(shm, sizeof(SharedData));
    shm_unlink("/shm_zad2");

    return 0;
}
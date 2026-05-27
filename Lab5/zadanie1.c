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
    Task queue[K];
    int head; // stad pobieramy dane
    int tail; // tu zapisujemy dane
    int count; // ile jest aktualnie zadan w buforze
} SharedData;

void producer(SharedData *shm, sem_t *empty, sem_t *full, sem_t *mutex, int id) {
    // Inicjalizacja generatora liczb losowych unikalna dla kazdego procesu
    srand(time(NULL) ^ (getpid() << 16)); 
    while (1) {
        sem_wait(empty); // Czekaj, az w buforze bedzie przynajmniej jedno wolne miejsce
        
        // POCZATEK SEKCJI KRYTYCZNEJ PRODUCENTA
        // Blokujemy muteks, poniewaz modyfikujemy wspoldzielona strukture danych (shm)
        sem_wait(mutex); 
        
        Task new_task;
        for (int i = 0; i < 10; i++) new_task.data[i] = 'A' + (rand() % 26);
        new_task.data[10] = '\0';

        shm->queue[shm->tail] = new_task; // dodajemy zadanie
        shm->tail = (shm->tail + 1) % K; // przesuwamy indeks w buforze cyklicznym
        shm->count++;
        printf("Producent %d Wygenerowano: %s\n", id, new_task.data);

        sem_post(mutex); // Odblokowanie dostepu do pamieci wspoldzielonej
        // KONIEC SEKCJI KRYTYCZNEJ PRODUCENTA
        
        sem_post(full);  // Zwiekszenie licznika gotowych zadan (budzenie konsumenta)
        sleep(1);
    }
}

void consumer(SharedData *shm, sem_t *empty, sem_t *full, sem_t *mutex, int id) {
    while (1) {
        sem_wait(full);  // Czekaj, az w buforze pojawi sie przynajmniej jedno zadanie
        
        // POCZATEK SEKCJI KRYTYCZNEJ KONSUMENTA 
        // Rezerwacja wylacznego dostepu do struktur przed pobraniem elementu
        sem_wait(mutex); 
        
        Task my_task = shm->queue[shm->head];
        shm->head = (shm->head + 1) % K; // przesuniecie indeksu czytania
        shm->count--;

        sem_post(mutex); // Odblokowanie dostepu do pamieci wspoldzielonej
        // KONIEC SEKCJI KRYTYCZNEJ KONSUMENTA
        
        sem_post(empty); // Zwiekszenie licznika wolnych miejsc (budzenie producenta)

        // Przetwarzanie i wypisywanie danych ZAWSZE poza sekcja krytyczna.
        // Gdyby usleep znalazl sie w sekcji krytycznej, zablokowalby caly system na 3 sekundy.
        printf("[Konsument %d] Pobrano: ", id);
        for (int i = 0; i < 10; i++) {
            printf("%c", my_task.data[i]);
            fflush(stdout);
            usleep(300000); // Emulacja ciezkiej pracy (0.3s na znak)
        }
        printf("\n");
    }
}

int main() {
    // Alokacja segmentu pamieci wspoldzielonej w systemie operacyjnym
    int shm_fd = shm_open("/shm_zad1", O_CREAT | O_RDWR, 0666); // Tworzymy miejsce w pamieci
    ftruncate(shm_fd, sizeof(SharedData)); // ustawiamy rozmiar na potrzebny przez nas
    
    // Mapowanie pamieci wspoldzielonej do przestrzeni adresowej tego procesu
    SharedData *shm = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0); // mapujemy wskaznik zeby mozna sie bylo odniesc
    memset(shm, 0, sizeof(SharedData)); // czyscimy pamiec

    // Czyszczenie starych semaforow na wypadek, gdyby poprzedni bieg programu ich nie zamknal
    sem_unlink("/sem_empty1"); sem_unlink("/sem_full1"); sem_unlink("/sem_mutex1");
    
    // Inicjalizacja semaforow:
    // empty = K (na starcie jest K wolnych miejsc)
    // full = 0 (na starcie nie ma zadnych gotowych zadan)
    // mutex = 1 (zapewnia wzajemne wykluczanie - dziala zero-jedynkowo)
    sem_t *empty = sem_open("/sem_empty1", O_CREAT, 0666, K); // ile wolnych miejsc
    sem_t *full = sem_open("/sem_full1", O_CREAT, 0666, 0); // ile gotowych zadan 
    sem_t *mutex = sem_open("/sem_mutex1", O_CREAT, 0666, 1);

    // Tworzenie N procesow producentow
    for (int i = 0; i < N; i++) 
    if (fork() == 0) {
         producer(shm, empty, full, mutex, i + 1); 
         exit(0); // Zabezpieczenie, zeby proces potomny nie kontynuowal petli main
    }

    // Tworzenie M procesow konsumentow
    for (int i = 0; i < M; i++) 
    if (fork() == 0) { 
        consumer(shm, empty, full, mutex, i + 1); 
        exit(0); 
    }
    
    // Proces macierzysty oczekuje na zakonczenie wszystkich uruchomionych dzieci
    for (int i = 0; i < N + M; i++) {
        wait(NULL);
    }

    // Sprzatanie zasobow systemowych po zakonczeniu dzialania
    sem_close(empty);
    sem_close(full);
    sem_close(mutex);
    munmap(shm, sizeof(SharedData));
    shm_unlink("/shm_zad1");

    return 0;
}
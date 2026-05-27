#ifdef LEVEL1

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include "common.h"

// Flaga mowiaca watkom czy maja dalej dzialac w petli
static int system_running = 1;

// Sloty (bufor o rozmiarze 1) do przekazywania danych miedzy watkami
static CameraFrame left_slot;
static CameraFrame right_slot;
static StereoPair stereo_slot;
static RobotState state_slot;

// Semafory pilnujace czy slot jest pusty (mozna pisac) czy pelny (mozna czytac)
static sem_t sem_left_empty, sem_left_full;
static sem_t sem_right_empty, sem_right_full;
static sem_t sem_stereo_empty, sem_stereo_full;
static sem_t sem_state_empty, sem_state_full;

// Mutexy zabezpieczajace fizyczny zapis i odczyt ze slotow przed race condition
static pthread_mutex_t mtx_left, mtx_right, mtx_stereo, mtx_state;

void* left_camera_thread(void* arg) {
    unsigned int f_num = 1;
    while (system_running) {
        CameraFrame frame;
        clock_gettime(CLOCK_MONOTONIC, &frame.ts);
        frame.frame_num = f_num++;

        // Czekamy az sync_thread oprozni slot lewej kamery
        sem_wait(&sem_left_empty);
        
        // --- SEKCJA KRYTYCZNA LEWEJ KAMERY ---
        pthread_mutex_lock(&mtx_left);
        left_slot = frame;
        pthread_mutex_unlock(&mtx_left);
        // --- KONIEC SEKCJI KRYTYCZNEJ ---
        
        // Trąbimy watkowi sync_thread, ze ma nowa ramke do pobrania
        sem_post(&sem_left_full);

        // Czestotliwosc 25 Hz to dokladnie 40 milisekund snu
        sleep_ns(40000000L); 
    }
    return NULL;
}

void* right_camera_thread(void* arg) {
    unsigned int f_num = 1;
    while (system_running) {
        CameraFrame frame;
        clock_gettime(CLOCK_MONOTONIC, &frame.ts);
        frame.frame_num = f_num++;

        // Czekamy az sync_thread oprozni slot prawej kamery
        sem_wait(&sem_right_empty);
        
        // --- SEKCJA KRYTYCZNA PRAWEJ KAMERY ---
        pthread_mutex_lock(&mtx_right);
        right_slot = frame;
        pthread_mutex_unlock(&mtx_right);
        // --- KONIEC SEKCJI KRYTYCZNEJ ---
        
        // Informujemy sync_thread, ze prawa ramka czeka
        sem_post(&sem_right_full);

        // Też 25 Hz, czyli 40 ms snu
        sleep_ns(40000000L); 
    }
    return NULL;
}

void* sync_thread(void* arg) {
    unsigned int pair_counter = 1;
    while (system_running) {
        // KROK 1: Pobieramy dane z lewej kamery
        sem_wait(&sem_left_full); // Czekamy na pelny slot lewy
        pthread_mutex_lock(&mtx_left);
        CameraFrame left = left_slot;
        pthread_mutex_unlock(&mtx_left);
        sem_post(&sem_left_empty); // Owalniamy slot lewy dla kamery

        // KROK 2: Pobieramy dane z prawej kamery
        sem_wait(&sem_right_full); // Czekamy na pelny slot prawy
        pthread_mutex_lock(&mtx_right);
        CameraFrame right = right_slot;
        pthread_mutex_unlock(&mtx_right);
        sem_post(&sem_right_empty); // Zwalniamy slot prawy dla kamery

        // KROK 3: Porownujemy timestampy (roznica musi byc mniejsza niz 20ms)
        if (get_timespec_diff(left.ts, right.ts) < 0.020) {
            StereoPair pair;
            pair.left_ts = left.ts;
            pair.right_ts = right.ts;
            pair.pair_num = pair_counter++;

            // KROK 4: Pchamy zsynchronizowana pare do watku zapisu (writer)
            sem_wait(&sem_stereo_empty); // Czekamy na wolne miejsce w slocie stereo
            
            // --- SEKCJA KRYTYCZNA SYNCHRONIZATORA ---
            pthread_mutex_lock(&mtx_stereo);
            stereo_slot = pair;
            pthread_mutex_unlock(&mtx_stereo);
            // --- KONIEC SEKCJI KRYTYCZNEJ ---
            
            sem_post(&sem_stereo_full); // Budzimy watek zapisu
        }
    }
    return NULL;
}

void* image_writer_thread(void* arg) {
    while (system_running) {
        // Czekamy az sync_thread wrzuci zsynchronizowana pare ram
        sem_wait(&sem_stereo_full);
        pthread_mutex_lock(&mtx_stereo);
        StereoPair pair = stereo_slot;
        pthread_mutex_unlock(&mtx_stereo);
        sem_post(&sem_stereo_empty); // Zwalniamy slot stereo

        // Tworzymy nazwy plikow wg wzoru z zadania
        char left_name[64];
        char right_name[64];
        sprintf(left_name, "left_%04u.txt", pair.pair_num);
        sprintf(right_name, "right_%04u.txt", pair.pair_num);

        // Zapisujemy lewy obraz (jako plik tekstowy)
        FILE* f_left = fopen(left_name, "w");
        if (f_left) {
            fprintf(f_left, "TS: %f\n", timespec_to_sec(pair.left_ts));
            fclose(f_left);
        }

        // Zapisujemy prawy obraz (jako plik tekstowy)
        FILE* f_right = fopen(right_name, "w");
        if (f_right) {
            fprintf(f_right, "TS: %f\n", timespec_to_sec(pair.right_ts));
            fclose(f_right);
        }

        // Czestotliwosc zapisu: 10 Hz = 100 milisekund snu
        sleep_ns(100000000L); 
    }
    return NULL;
}

void* robot_state_thread(void* arg) {
    double dummy_val = 0.0;
    while (system_running) {
        // Generujemy sztuczne dane o pozycji i orientacji robota
        RobotState state;
        state.x = dummy_val;
        state.y = dummy_val * 1.1;
        state.z = 0.0;
        state.roll = 0.0;
        state.pitch = 0.0;
        state.yaw = dummy_val * 0.05;
        clock_gettime(CLOCK_MONOTONIC, &state.ts);
        dummy_val += 0.1;

        // Przekazujemy stan do watku loggera przez state_slot
        sem_wait(&sem_state_empty);
        
        // --- SEKCJA KRYTYCZNEGO ZAPISU STANU ---
        pthread_mutex_lock(&mtx_state);
        state_slot = state;
        pthread_mutex_unlock(&mtx_state);
        // --- KONIEC SEKCJI KRYTYCZNEJ ---
        
        sem_post(&sem_state_full); // Budzimy loggera, dane sa gotowe

        // Wysoki priorytet: 100 Hz = dokladnie 10 milisekund snu
        sleep_ns(10000000L); 
    }
    return NULL;
}

void* logger_thread(void* arg) {
    FILE* log_file = fopen("robot_state.txt", "w");
    if (!log_file) return NULL;

    while (system_running) {
        // Czekamy na nowy stan od watku robot_state
        sem_wait(&sem_state_full);
        pthread_mutex_lock(&mtx_state);
        RobotState state = state_slot;
        pthread_mutex_unlock(&mtx_state);
        sem_post(&sem_state_empty); // Oprozniamy slot dla watku stanu

        // Zapis danych do pliku robot_state.txt
        fprintf(log_file, "[%f] Pos: (%.2f, %.2f, %.2f) Yaw: %.2f\n", 
                timespec_to_sec(state.ts), state.x, state.y, state.z, state.yaw);
        fflush(log_file); // Wymuszamy natychmiastowy zapis na dysk

        // Czestotliwosc loggera: 10 Hz = 100 milisekund snu
        sleep_ns(100000000L); 
    }

    fclose(log_file);
    return NULL;
}

int main() {
    // Inicjalizacja semaforow (drugi parametr 0 oznacza ze sa dla watkow, a nie procesow)
    sem_init(&sem_left_empty, 0, 1);  // Na starcie slot jest pusty (1 wolne miejsce)
    sem_init(&sem_left_full, 0, 0);   // Na starcie slot nie ma danych (0 pelnych)
    sem_init(&sem_right_empty, 0, 1);
    sem_init(&sem_right_full, 0, 0);
    sem_init(&sem_stereo_empty, 0, 1);
    sem_init(&sem_stereo_full, 0, 0);
    sem_init(&sem_state_empty, 0, 1);
    sem_init(&sem_state_full, 0, 0);

    // Inicjalizacja mutexow do ochrony pamieci
    pthread_mutex_init(&mtx_left, NULL);
    pthread_mutex_init(&mtx_right, NULL);
    pthread_mutex_init(&mtx_stereo, NULL);
    pthread_mutex_init(&mtx_state, NULL);

    pthread_t t_left_cam, t_right_cam, t_sync, t_writer, t_state, t_logger;

    // Odpalamy wszystkie watki skladowe systemu
    pthread_create(&t_left_cam, NULL, left_camera_thread, NULL);
    pthread_create(&t_right_cam, NULL, right_camera_thread, NULL);
    pthread_create(&t_sync, NULL, sync_thread, NULL);
    pthread_create(&t_writer, NULL, image_writer_thread, NULL);
    pthread_create(&t_state, NULL, robot_state_thread, NULL);
    pthread_create(&t_logger, NULL, logger_thread, NULL);

    // Glowny watek spi przez zadane w instrukcji 20 sekund
    sleep(20);

    // Zgłaszamy watkom, ze pora konczyc zabewe
    system_running = 0;

    // Sztucznie podbijamy semafory (tzw. "kopniaki"), zeby odblokowac watki wiszace na sem_wait
    sem_post(&sem_left_empty);
    sem_post(&sem_left_full);
    sem_post(&sem_right_empty);
    sem_post(&sem_right_full);
    sem_post(&sem_stereo_empty);
    sem_post(&sem_stereo_full);
    sem_post(&sem_state_empty);
    sem_post(&sem_state_full);

    // Czekamy na eleganckie zamkniecie i posprzatane kazdego watku po kolei
    pthread_join(t_left_cam, NULL);
    pthread_join(t_right_cam, NULL);
    pthread_join(t_sync, NULL);
    pthread_join(t_writer, NULL);
    pthread_join(t_state, NULL);
    pthread_join(t_logger, NULL);

    // Destrukcja semaforow i czyszczenie zasobow z systemu
    sem_destroy(&sem_left_empty);
    sem_destroy(&sem_left_full);
    sem_destroy(&sem_right_empty);
    sem_destroy(&sem_right_full);
    sem_destroy(&sem_stereo_empty);
    sem_destroy(&sem_stereo_full);
    sem_destroy(&sem_state_empty);
    sem_destroy(&sem_state_full);

    // Destrukcja mutexow
    pthread_mutex_destroy(&mtx_left);
    pthread_mutex_destroy(&mtx_right);
    pthread_mutex_destroy(&mtx_stereo);
    pthread_mutex_destroy(&mtx_state);

    return 0;
}

#endif
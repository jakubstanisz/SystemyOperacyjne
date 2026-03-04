#include "definitions.h"
#include <sys/file.h> // Potrzebne do funkcji flock

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 1;
    }

    int M = atoi(argv[1]);

    for (int i = 0; i < M; i++) {
        // Otwieramy plik w trybie "a" (append - dopisywanie)
        FILE *file = fopen(OUTPUT_FILE, "a");
        if (file == NULL) {
            perror("Błąd otwarcia pliku");
            exit(1);
        }
        // zabrane z upel dokumentacji
        // Blokada wyłączna (exclusive lock — LOCK_EX)
        // Tylko jeden proces może uzyskać dostęp do pliku (odczyt i zapis) w danym momencie.
        // flock dziala na deskryptorze pliku, który pobieramy przez fileno()
        flock(fileno(file), LOCK_EX);

        // zapis do pliku zamiast na ekran
        fprintf(file, "Potomek (PID: %d)\n", getpid());

        //wypchniecie danych z buffora na dysk
        fflush(file);

        // zdejmujemy blokade
        flock(fileno(file), LOCK_UN);

        fclose(file);

        // Czekamy 0.25 sekundy
        usleep(250000);
    }

    return 0;
}
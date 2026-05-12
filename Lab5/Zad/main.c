#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <time.h>
#include <sys/wait.h>

#define N 3
#define M 2
#define K 10
#define STR_LEN 11

typedef struct {
    char data[STR_LEN];
} Task;

typedef struct {
    Task normal_queue[K];
    int normal_head;
    int normal_tail;
    int normal_count;
    
    Task priority_queue[K];
    int priority_head;
    int priority_tail;
    int priority_count;
} SharedData;

void generate_string(char *str) {
    for (int i = 0; i < 10; i++) {
        str[i] = 'A' + (rand() % 26);
    }
    str[10] = '\0';
}

int main() {
    int shm_fd = shm_open("/shm_buffer_zadania", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(SharedData));
    SharedData *shared = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    shared->normal_head = 0;
    shared->normal_tail = 0;
    shared->normal_count = 0;
    shared->priority_head = 0;
    shared->priority_tail = 0;
    shared->priority_count = 0;

    sem_t *sem_empty = sem_open("/sem_empty", O_CREAT, 0666, K);
    sem_t *sem_full = sem_open("/sem_full", O_CREAT, 0666, 0);
    sem_t *sem_mutex = sem_open("/sem_mutex", O_CREAT, 0666, 1);

    for (int i = 0; i < N; i++) {
        if (fork() == 0) {
            srand(time(NULL) ^ (getpid() << 16));
            while (1) {
                sem_wait(sem_empty);
                sem_wait(sem_mutex);

                Task t;
                generate_string(t.data);
                
                if (rand() % 100 < 30) {
                    shared->priority_queue[shared->priority_tail] = t;
                    shared->priority_tail = (shared->priority_tail + 1) % K;
                    shared->priority_count++;
                } else {
                    shared->normal_queue[shared->normal_tail] = t;
                    shared->normal_tail = (shared->normal_tail + 1) % K;
                    shared->normal_count++;
                }

                sem_post(sem_mutex);
                sem_post(sem_full);
                sleep(1);
            }
            exit(0);
        }
    }

    for (int i = 0; i < M; i++) {
        if (fork() == 0) {
            while (1) {
                sem_wait(sem_full);
                sem_wait(sem_mutex);

                Task t;
                if (shared->priority_count > 0) {
                    t = shared->priority_queue[shared->priority_head];
                    shared->priority_head = (shared->priority_head + 1) % K;
                    shared->priority_count--;
                } else {
                    t = shared->normal_queue[shared->normal_head];
                    shared->normal_head = (shared->normal_head + 1) % K;
                    shared->normal_count--;
                }

                sem_post(sem_mutex);
                sem_post(sem_empty);

                for (int j = 0; j < 10; j++) {
                    printf("%c", t.data[j]);
                    fflush(stdout);
                    usleep(300000);
                }
                printf("\n");
            }
            exit(0);
        }
    }

    if (fork() == 0) {
        while (1) {
            sleep(5);
            sem_wait(sem_mutex);
            
            if (shared->normal_count > 0) {
                Task t = shared->normal_queue[shared->normal_head];
                shared->normal_head = (shared->normal_head + 1) % K;
                shared->normal_count--;

                shared->priority_queue[shared->priority_tail] = t;
                shared->priority_tail = (shared->priority_tail + 1) % K;
                shared->priority_count++;
            }

            printf("\n[Manager] Status systemu: NORMAL=%d, PRIORITY=%d\n", shared->normal_count, shared->priority_count);
            
            sem_post(sem_mutex);
        }
        exit(0);
    }

    for (int i = 0; i < N + M + 1; i++) {
        wait(NULL);
    }

    munmap(shared, sizeof(SharedData));
    shm_unlink("/shm_buffer_zadania");
    sem_close(sem_empty);
    sem_close(sem_full);
    sem_close(sem_mutex);
    sem_unlink("/sem_empty");
    sem_unlink("/sem_full");
    sem_unlink("/sem_mutex");

    return 0;
}
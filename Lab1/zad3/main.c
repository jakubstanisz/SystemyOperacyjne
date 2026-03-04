#include "definitions.h"
int main(int argc, char *argv[]){
    //usuwamy plik output.txt ktory jest zdefiniowany jako OUTPUT_FILE
    remove(OUTPUT_FILE);
    if (argc < 3) {
        return 1;
    }
    
    int N = atoi(argv[1]);
    
    pid_t pid;
    
    for (int i = 0; i < N; i++){
        pid = fork();
        if (pid == 0) {
            execl("./child", "child", argv[2], NULL);
            return 1;
        }
    }
    for (int i = 0; i < N; i++){
        wait(NULL);
    }
    printf("Rodzic  (PID: %d)\n", getpid());
    return 0;
}
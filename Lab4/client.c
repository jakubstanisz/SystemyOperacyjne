#include "common.h"

int client_queue;
char path[64];

void cleanup(int sig) {
    msgctl(client_queue, IPC_RMID, NULL);
    unlink(path);
    exit(0);
}

int main() {
    signal(SIGINT, cleanup);

    key_t server_key = ftok(SERVER_KEY_PATHNAME, SERVER_PROJ_ID);
    int server_queue = msgget(server_key, 0);

    sprintf(path, "/tmp/client_%d", getpid());
    FILE *f = fopen(path, "w");
    if (f) fclose(f);

    key_t client_key = ftok(path, 'C');
    client_queue = msgget(client_key, IPC_CREAT | 0666);

    struct message msg;
    msg.mtype = MSG_INIT;
    msg.queue_key = client_key;
    msgsnd(server_queue, &msg, sizeof(struct message) - sizeof(long), 0);

    msgrcv(client_queue, &msg, sizeof(struct message) - sizeof(long), MSG_INIT, 0);
    int my_id = msg.client_id;

    pid_t pid = fork();

    if (pid == 0) {
        while (1) { 
            if (msgrcv(client_queue, &msg, sizeof(struct message) - sizeof(long), MSG_TEXT, 0) > 0) {
                printf("Klient %d: %s", msg.client_id, msg.text);
            }
        }
    } else {
        while (1) { 
            char buffer[MAX_TEXT];
            if (fgets(buffer, MAX_TEXT, stdin) != NULL) {
                msg.mtype = MSG_TEXT;
                msg.client_id = my_id;
                strcpy(msg.text, buffer);
                msgsnd(server_queue, &msg, sizeof(struct message) - sizeof(long), 0);
            }
        }
    }

    return 0;
}
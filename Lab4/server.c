#include "common.h"

int server_queue;

void cleanup(int sig) {
    msgctl(server_queue, IPC_RMID, NULL);
    exit(0);
}

int main() {
    signal(SIGINT, cleanup);

    key_t server_key = ftok(SERVER_KEY_PATHNAME, SERVER_PROJ_ID);
    server_queue = msgget(server_key, IPC_CREAT | 0666);

    int client_queues[MAX_CLIENTS];
    int client_count = 0;
    struct message msg;

    while (1) {
        msgrcv(server_queue, &msg, sizeof(struct message) - sizeof(long), 0, 0);

        if (msg.mtype == MSG_INIT) {
            if (client_count < MAX_CLIENTS) {
                int client_q = msgget(msg.queue_key, 0);
                client_queues[client_count] = client_q;

                msg.mtype = MSG_INIT;
                msg.client_id = client_count;
                msgsnd(client_q, &msg, sizeof(struct message) - sizeof(long), 0);

                client_count++;
            }
        } else if (msg.mtype == MSG_TEXT) {
            for (int i = 0; i < client_count; i++) {
                if (i != msg.client_id) {
                    msgsnd(client_queues[i], &msg, sizeof(struct message) - sizeof(long), 0);
                }
            }
        }
    }
    return 0;
}
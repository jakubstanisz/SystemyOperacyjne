#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>

#define SERVER_KEY_PATHNAME "/tmp"
#define SERVER_PROJ_ID 'S'
#define MAX_CLIENTS 10
#define MAX_TEXT 1024

#define MSG_INIT 1
#define MSG_TEXT 2

struct message {
    long mtype;
    int client_id;
    key_t queue_key;
    char text[MAX_TEXT];
};

#endif
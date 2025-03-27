#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_MSG_SIZE 256
#define PERMS 0644

typedef struct {
    long mtype;
    char mtext[MAX_MSG_SIZE];
    int client_id;
} Message;

int msgqid;
int client_id;

void handle_signal(int sig) {
    printf("\nClient shutting down...\n");
    exit(0);
}

void send_command(const char* command) {
    Message msg;
    msg.mtype = 1;
    strncpy(msg.mtext, command, MAX_MSG_SIZE);
    msg.client_id = client_id;
    
    if (msgsnd(msgqid, &msg, sizeof(msg.mtext) + sizeof(int), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }
    
    Message reply;
    if (msgrcv(msgqid, &reply, sizeof(reply.mtext) + sizeof(int), client_id, 0) == -1) {
        perror("msgrcv");
        exit(1);
    }
    
    printf("Server reply: %s\n", reply.mtext);
    if (strcmp(reply.mtext, "game_over") == 0) {
        exit(0);
    }
}

void register_client() {
    Message msg;
    msg.mtype = 1;
    strcpy(msg.mtext, "register");
    msg.client_id = getpid();
    
    if (msgsnd(msgqid, &msg, sizeof(msg.mtext) + sizeof(int), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }
    
    Message reply;
    if (msgrcv(msgqid, &reply, sizeof(reply.mtext) + sizeof(int), getpid(), 0) == -1) {
        perror("msgrcv");
        exit(1);
    }
    
    if (strcmp(reply.mtext, "registered") == 0) {
        client_id = getpid();
        printf("Registered with id %d\n", client_id);
    } else {
        printf("Registration failed\n");
        exit(1);
    }
}

void process_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        exit(1);
    }
    
    char line[MAX_MSG_SIZE];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';
        
        if (strlen(line) == 0) continue;
        
        printf("Sending command: %s\n", line);
        send_command(line);
        sleep(1);
    }
    
    fclose(file);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <command_file>\n", argv[0]);
        exit(1);
    }
    
    key_t key = ftok("server.c", 'A');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }
    
    msgqid = msgget(key, PERMS);
    if (msgqid == -1) {
        perror("msgget");
        exit(1);
    }
    
    register_client();
    process_file(argv[1]);
    
    return 0;
}
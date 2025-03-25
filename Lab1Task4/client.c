#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#define MAX_MSG_SIZE 256

typedef struct {
    long mtype;
    char mtext[MAX_MSG_SIZE];
    int client_id;
} Message;

key_t server_key;
int server_msqid;
int client_id;

void generate_client_id() {
    srand(time(NULL) ^ getpid());
    client_id = rand() % 1000 + 1;
}

void connect_to_server() {
    server_key = ftok("server.c", 'A');
    if (server_key == -1) {
        perror("ftok");
        exit(1);
    }
    
    server_msqid = msgget(server_key, 0666);
    if (server_msqid == -1) {
        perror("msgget");
        exit(1);
    }
    
    printf("Connected to server with ID: %d\n", client_id);
}

void send_command(const char* command) {
    Message msg;
    msg.mtype = 1;
    msg.client_id = client_id;
    snprintf(msg.mtext, MAX_MSG_SIZE, "%s", command);
    
    if (msgsnd(server_msqid, &msg, sizeof(msg.mtext), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }
    
    printf("Sent command: %s\n", command);
}

void receive_response() {
    Message msg;
    if (msgrcv(server_msqid, &msg, sizeof(msg.mtext), client_id, 0) == -1) {
        perror("msgrcv");
        exit(1);
    }
    
    printf("Server response: %s\n", msg.mtext);
}

void process_commands_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        exit(1);
    }
    
    char line[MAX_MSG_SIZE];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';
        
        if (strlen(line) > 0) {
            send_command(line);
            receive_response();
            sleep(1);
        }
    }
    
    fclose(file);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <commands_file>\n", argv[0]);
        return 1;
    }
    
    generate_client_id();
    connect_to_server();
    process_commands_from_file(argv[1]);
    
    return 0;
}
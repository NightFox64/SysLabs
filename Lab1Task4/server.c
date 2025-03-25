#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define MAX_MSG_SIZE 256
#define MAX_CLIENTS 10

enum errors {
    SUCCESS = 0,
    ERROR_BAD_SITUATION = 1,
    ERROR_GAME_OVER = -2,
    ERROR_UNDEFINED_OBJECT = -3,
    ERROR_FULL_BOAT = -4,
    ERROR_WRONG_SHORE = -5,
    ERROR_NOTHING_TO_PUT = -6,
    ERROR_INVALID_COMMAND = -7,
};

typedef enum { 
    WOLF, 
    GOAT, 
    CABBAGE, 
    NONE 
} Object;

typedef struct {
    long mtype;
    char mtext[MAX_MSG_SIZE];
    int client_id;
} Message;

typedef struct {
    int client_id;
    pid_t pid;
    time_t last_active;
} ClientInfo;

typedef struct {
    Object left_shore[3];  // wolf, goat, cabbage
    Object right_shore[3];
    Object boat;
    int boat_position; // 0 - left, 1 - right
    int game_over;
    int success;
} GameState;

key_t server_key;
int server_msqid;
ClientInfo clients[MAX_CLIENTS];
int num_clients = 0;
GameState game_state;

void initialize_game() {
    game_state.left_shore[0] = WOLF;
    game_state.left_shore[1] = GOAT;
    game_state.left_shore[2] = CABBAGE;
    game_state.right_shore[0] = NONE;
    game_state.right_shore[1] = NONE;
    game_state.right_shore[2] = NONE;
    game_state.boat = NONE;
    game_state.boat_position = 0;
    game_state.game_over = 0;
    game_state.success = 0;
}

int check_victory() {
    return game_state.right_shore[0] == WOLF && 
           game_state.right_shore[1] == GOAT && 
           game_state.right_shore[2] == CABBAGE;
}

int check_loss() {
    if (game_state.boat_position == 1) {
        if (game_state.left_shore[0] == WOLF && game_state.left_shore[1] == GOAT && game_state.left_shore[2] != CABBAGE) {
            return ERROR_BAD_SITUATION;
        }
        if (game_state.left_shore[1] == GOAT && game_state.left_shore[2] == CABBAGE && game_state.left_shore[0] != WOLF) {
            return ERROR_BAD_SITUATION;
        }
    }
    else {
        if (game_state.right_shore[0] == WOLF && game_state.right_shore[1] == GOAT && game_state.right_shore[2] != CABBAGE) {
            return ERROR_BAD_SITUATION;
        }
        if (game_state.right_shore[1] == GOAT && game_state.right_shore[2] == CABBAGE && game_state.right_shore[0] != WOLF) {
            return ERROR_BAD_SITUATION;
        }
    }
    return SUCCESS;
}

void update_game_state() {
    if (check_victory()) {
        game_state.game_over = 1;
        game_state.success = 1;
    } else if (check_loss()) {
        game_state.game_over = 1;
        game_state.success = 0;
    }
}

int process_command(int client_id, const char* command) {
    if (game_state.game_over) {
        return ERROR_GAME_OVER;
    }

    char cmd[10];
    char obj[10];
    int parsed = sscanf(command, "%s %s", cmd, obj);
    
    if (strcmp(cmd, "take") == 0 && parsed == 2) {
        Object object = NONE;
        if (strcmp(obj, "wolf") == 0) object = WOLF;
        else if (strcmp(obj, "goat") == 0) object = GOAT;
        else if (strcmp(obj, "cabbage") == 0) object = CABBAGE;
        else return ERROR_UNDEFINED_OBJECT;
        
        if (game_state.boat != NONE) return ERROR_FULL_BOAT;
        
        Object* shore = game_state.boat_position ? game_state.right_shore : game_state.left_shore;
        if (shore[object] != object) return ERROR_WRONG_SHORE;
        
        shore[object] = NONE;
        game_state.boat = object;
    }
    else if (strcmp(cmd, "put") == 0 && parsed == 1) {
        if (game_state.boat == NONE) return ERROR_NOTHING_TO_PUT;
        
        Object* shore = game_state.boat_position ? game_state.right_shore : game_state.left_shore;
        shore[game_state.boat] = game_state.boat;
        game_state.boat = NONE;
    }
    else if (strcmp(cmd, "move") == 0 && parsed == 1) {
        game_state.boat_position = !game_state.boat_position;
    }
    else {
        return ERROR_INVALID_COMMAND;
    }
    
    update_game_state();
    return SUCCESS;
}

void send_response(int client_id, const char* response) {
    Message msg;
    msg.mtype = client_id;
    msg.client_id = client_id;
    snprintf(msg.mtext, MAX_MSG_SIZE, "%s", response);
    
    if (msgsnd(server_msqid, &msg, sizeof(msg.mtext), 0) == -1) {
        perror("msgsnd");
    }
}

void handle_client_message(Message* msg) {
    int client_id = msg->client_id;
    char response[MAX_MSG_SIZE];
    
    int result = process_command(client_id, msg->mtext);
    
    if (result == SUCCESS) {
        if (game_state.game_over) {
            if (game_state.success) {
                snprintf(response, MAX_MSG_SIZE, "Success! All items are safely on the right shore.");
            } else {
                snprintf(response, MAX_MSG_SIZE, "Game over! Something was eaten.");
            }
        } else {
            snprintf(response, MAX_MSG_SIZE, "Command executed. State: %s shore - wolf:%d goat:%d cabbage:%d | boat:%d | %s shore - wolf:%d goat:%d cabbage:%d",
                     game_state.boat_position ? "left" : "right",
                     game_state.left_shore[0] != NONE, game_state.left_shore[1] != NONE, game_state.left_shore[2] != NONE,
                     game_state.boat != NONE,
                     game_state.boat_position ? "right" : "left",
                     game_state.right_shore[0] != NONE, game_state.right_shore[1] != NONE, game_state.right_shore[2] != NONE);
        }
    } else {
        switch (result) {
            case ERROR_GAME_OVER: strcpy(response, "Game is already over"); break;
            case ERROR_UNDEFINED_OBJECT: strcpy(response, "Invalid object"); break;
            case ERROR_FULL_BOAT: strcpy(response, "Boat is full"); break;
            case ERROR_WRONG_SHORE: strcpy(response, "Object not on this shore"); break;
            case ERROR_NOTHING_TO_PUT: strcpy(response, "Nothing to put"); break;
            case ERROR_INVALID_COMMAND: strcpy(response, "Invalid command"); break;
            default: strcpy(response, "Unknown error"); break;
        }
    }
    
    send_response(client_id, response);
}

void cleanup() {
    msgctl(server_msqid, IPC_RMID, NULL);
    printf("Server shutdown\n");
    exit(0);
}

int main() {
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    
    server_key = ftok("server.c", 'A');
    if (server_key == -1) {
        perror("ftok");
        exit(1);
    }
    
    server_msqid = msgget(server_key, IPC_CREAT | 0666);
    if (server_msqid == -1) {
        perror("msgget");
        exit(1);
    }
    
    initialize_game();
    printf("Server started. Waiting for messages...\n");
    
    while (1) {
        Message msg;
        if (msgrcv(server_msqid, &msg, sizeof(msg.mtext), 1, 0) == -1) {
            perror("msgrcv");
            continue;
        }
        
        int client_id = msg.client_id;
        int client_exists = 0;
        
        for (int i = 0; i < num_clients; i++) {
            if (clients[i].client_id == client_id) {
                client_exists = 1;
                clients[i].last_active = time(NULL);
                break;
            }
        }
        
        if (!client_exists && num_clients < MAX_CLIENTS) {
            clients[num_clients].client_id = client_id;
            clients[num_clients].last_active = time(NULL);
            num_clients++;
            printf("New client connected: %d\n", client_id);
        }
        
        printf("Received from client %d: %s\n", client_id, msg.mtext);
        handle_client_message(&msg);
    }
    
    return SUCCESS;
}
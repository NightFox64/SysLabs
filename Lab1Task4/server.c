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
#define PERMS 0644

typedef enum { WOLF, GOAT, CABBAGE, NONE } Object;

typedef struct {
    Object left_shore[3];  // wolf, goat, cabbage
    Object right_shore[3];
    Object boat;
    int boat_position; // 0 - left, 1 - right
    int game_over;
} GameState;

typedef struct {
    long mtype;
    char mtext[MAX_MSG_SIZE];
    int client_id;
} Message;

typedef struct {
    int client_id;
    pid_t pid;
    int active;
} ClientInfo;

int msgqid;
ClientInfo clients[MAX_CLIENTS];
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
}

int check_game_over() {
    // Check left shore
    if (game_state.boat_position == 1) {
        int wolf_left = game_state.left_shore[0] == WOLF;
        int goat_left = game_state.left_shore[1] == GOAT;
        int cabbage_left = game_state.left_shore[2] == CABBAGE;
        
        if (wolf_left && goat_left) {
            printf("Wolf ate goat on left shore!\n");
            return 1;
        }
        if (goat_left && cabbage_left) {
            printf("Goat ate cabbage on left shore!\n");
            return 1;
        }
    }
    
    // Check right shore
    if (game_state.boat_position == 0) {
        int wolf_right = game_state.right_shore[0] == WOLF;
        int goat_right = game_state.right_shore[1] == GOAT;
        int cabbage_right = game_state.right_shore[2] == CABBAGE;
        
        if (wolf_right && goat_right) {
            printf("Wolf ate goat on right shore!\n");
            return 1;
        }
        if (goat_right && cabbage_right) {
            printf("Goat ate cabbage on right shore!\n");
            return 1;
        }
    }
    
    // Check win condition
    if (game_state.right_shore[0] == WOLF &&
        game_state.right_shore[1] == GOAT &&
        game_state.right_shore[2] == CABBAGE) {
        printf("All objects are safely on the right shore! You win!\n");
        return 2;
    }
    
    return 0;
}

void print_state() {
    printf("\nCurrent state:\n");
    printf("Left shore: ");
    if (game_state.left_shore[0] == WOLF) printf("wolf ");
    if (game_state.left_shore[1] == GOAT) printf("goat ");
    if (game_state.left_shore[2] == CABBAGE) printf("cabbage ");
    
    printf("\nRight shore: ");
    if (game_state.right_shore[0] == WOLF) printf("wolf ");
    if (game_state.right_shore[1] == GOAT) printf("goat ");
    if (game_state.right_shore[2] == CABBAGE) printf("cabbage ");
    
    printf("\nBoat is on the %s shore. ", game_state.boat_position ? "right" : "left");
    printf("Boat contains: ");
    if (game_state.boat == WOLF) printf("wolf");
    else if (game_state.boat == GOAT) printf("goat");
    else if (game_state.boat == CABBAGE) printf("cabbage");
    else printf("nothing");
    printf("\n\n");
}

int process_command(int client_id, const char* command) {
    if (game_state.game_over) {
        return -1;
    }
    
    char cmd[10];
    char obj[10];
    int args = sscanf(command, "%s %s", cmd, obj);
    
    if (args == 1 && strcmp(cmd, "put") == 0) {
        if (game_state.boat == NONE) {
            printf("Client %d: Boat is empty, nothing to put\n", client_id);
            return 0;
        }
        
        Object item = game_state.boat;
        game_state.boat = NONE;
        
        if (game_state.boat_position == 0) { // left shore
            game_state.left_shore[item] = item;
        } else { // right shore
            game_state.right_shore[item] = item;
        }

        int result = check_game_over();                 //  MAYBE NEED TO REMOVE
        if (result == 1) {
            game_state.game_over = 1;
            printf("Game over! You lost!\n");
            return -1;
        } else if (result == 2) {
            game_state.game_over = 1;
            printf("Game over! You won!\n");
            return -1;
        }
        
        printf("Client %d: Put %d on shore\n", client_id, item);
        return 1;
    }
    else if (args == 2 && strcmp(cmd, "take") == 0) {
        if (game_state.boat != NONE) {
            printf("Client %d: Boat is already full\n", client_id);
            return 0;
        }
        
        Object item;
        if (strcmp(obj, "wolf") == 0) item = WOLF;
        else if (strcmp(obj, "goat") == 0) item = GOAT;
        else if (strcmp(obj, "cabbage") == 0) item = CABBAGE;
        else {
            printf("Client %d: Unknown object '%s'\n", client_id, obj);
            return 0;
        }
        
        int* shore = game_state.boat_position ? game_state.right_shore : game_state.left_shore;
        if (shore[item] != item) {
            printf("Client %d: Object %d not on this shore\n", client_id, item);
            return 0;
        }
        
        shore[item] = NONE;
        game_state.boat = item;
        printf("Client %d: Took %d into boat\n", client_id, item);
        return 1;
    }
    else if (args == 1 && strcmp(cmd, "move") == 0) {
        game_state.boat_position = !game_state.boat_position;
        printf("Client %d: Moved boat to %s shore\n", client_id, 
               game_state.boat_position ? "right" : "left");
        
        int result = check_game_over();
        if (result == 1) {
            game_state.game_over = 1;
            printf("Game over! You lost!\n");
            return -1;
        } else if (result == 2) {
            game_state.game_over = 1;
            printf("Game over! You won!\n");
            return -1;
        }
        
        return 1;
    }
    else {
        printf("Client %d: Unknown command '%s'\n", client_id, command);
        return 0;
    }
}

void handle_signal(int sig) {
    printf("\nServer shutting down...\n");
    msgctl(msgqid, IPC_RMID, NULL);
    exit(0);
}

int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    key_t key = ftok("server.c", 'A');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }
    
    msgqid = msgget(key, PERMS | IPC_CREAT);
    if (msgqid == -1) {
        perror("msgget");
        exit(1);
    }
    
    printf("Server started with message queue id %d\n", msgqid);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].client_id = -1;
        clients[i].active = 0;
    }
    
    initialize_game();
    print_state();
    
    while (1) {
        Message msg;
        if (msgrcv(msgqid, &msg, sizeof(msg.mtext) + sizeof(int), 0, 0) == -1) {
            perror("msgrcv");
            continue;
        }
        
        printf("Received message from client %d: %s\n", msg.client_id, msg.mtext);
        
        if (strcmp(msg.mtext, "register") == 0) {
            int found = 0;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (!clients[i].active) {
                    clients[i].client_id = msg.client_id;
                    clients[i].pid = msg.client_id; // Using client_id as PID for simplicity
                    clients[i].active = 1;
                    found = 1;
                    
                    Message reply;
                    reply.mtype = msg.client_id;
                    strcpy(reply.mtext, "registered");
                    reply.client_id = 0;
                    
                    if (msgsnd(msgqid, &reply, sizeof(reply.mtext) + sizeof(int), 0) == -1) {
                        perror("msgsnd");
                    }
                    
                    printf("Registered new client with id %d\n", msg.client_id);
                    break;
                }
            }
            
            if (!found) {
                printf("Max clients reached, cannot register new client\n");
            }
        } else {
            int result = process_command(msg.client_id, msg.mtext);
            
            Message reply;
            reply.mtype = msg.client_id;
            reply.client_id = 0;
            
            if (result == 1) {
                strcpy(reply.mtext, "success");
                print_state();
            } else if (result == 0) {
                strcpy(reply.mtext, "error");
            } else if (result == -1) {
                strcpy(reply.mtext, "game_over");
            }
            
            if (msgsnd(msgqid, &reply, sizeof(reply.mtext) + sizeof(int), 0) == -1) {
                perror("msgsnd");
            }
        }
    }
    
    return 0;
}   
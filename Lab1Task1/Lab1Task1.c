#include "functions.h"

int main() {
    int flagExit = 0;
    while (!flagExit) {
        printf("Welcome to the shell. Please log in or register.\n");
        
        int choice;
        printf("1. Register\n2. Login\n3. Exit\n");
        if (scanf("%d", &choice) != 1) {
            printf("Input must be a number\n");
            return ERROR_INPUT_CHOICE;
        } 

        int flagLogin = 0;

        if (choice == 1) {
            int code = register_user();
            if (code != 0) return code;
            continue;
        } else if (choice == 2) {
            int user_index = login_user();
            flagLogin++;
            if (user_index == -1) continue;
            User *current_user = &users[user_index];

            while (1) {
                if (flagLogin == 0) {
                    printf("> ");
                }
                char command[MAX_COMMAND_LEN];
                fgets(command, sizeof(command), stdin);

                size_t len = strlen(command);
                if (len > 0 && command[len - 1] == '\n') {
                    command[len - 1] = '\0';
                }
                if (strcmp(command, "Time") == 0) {
                    display_time();
                } else if (strcmp(command, "Date") == 0) {
                    display_date();
                } else if (strncmp(command, "Howmuch ", 8) == 0) {
                    char time_str[20];
                    char flag[3];
                    struct tm start_date = {0};
                    parse_input(command + 8, &start_date, flag);
                    time_t start_time = mktime(&start_date);
                    long long result = howmuch(start_time, flag);
                    printf("Time delta is: %lld\n", result);
                } else if (strcmp(command, "Logout") == 0) {
                    printf("Logging out...\n");
                    break;
                } else if (strncmp(command, "Sanctions ", 10) == 0) {
                    char username[MAX_LOGIN_LEN];
                    int limit;
                    sscanf(command + 10, "%6s %d", username, &limit);
                    apply_sanctions(current_user, username, limit);
                    flagLogin = 1;
                } else {
                    if (flagLogin == 0) {
                        printf("Unknown command.\n");
                    }
                    else {
                        flagLogin = 0;
                    }
                }

                if (current_user->request_limit != -1) {
                    current_user->request_count++;
                    if (current_user->request_count >= current_user->request_limit) {
                        printf("Request limit reached. Logging out...\n");
                        break;
                    }
                }
            }
        } else if (choice == 3) {
            flagExit = 1;
            break;
        } else {
            printf("Invalid choice.\n");
        }
    }
    
    return SUCCESS;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_USERS 1000
#define MAX_LOGIN_LEN 7
#define MAX_PIN_LEN 6
#define MAX_COMMAND_LEN 100

enum errors {
    SUCCESS = 0,
    ERROR_NO_USER = -1,
    ERROR_USERS_LIMIT = -2,
    ERROR_USER_ALREADY_EXIST = -3,
    ERROR_INVALID_PIN = -4,
    ERROR_INVALID_FLAG = -5,
    ERROR_TIME_PARSING = -6,
    ERROR_INPUT_CHOICE = -7,
    ERROR_TRASH_IN_LOGIN = -8,
};

typedef struct {
    char login[MAX_LOGIN_LEN];
    int pin;
    int request_limit;
    int request_count;
} User;

User users[MAX_USERS];
int user_count = 0;

int find_user(const char *login) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].login, login) == 0) {
            return i;
        }
    }
    return ERROR_NO_USER;
}

int is_trash(char symbol) {
    if ((symbol >= '0') && (symbol <= '9')) {
        return 0;
    }
    else if ((symbol >= 'a') && (symbol <= 'z')) {
        return 0;
    }
    else if ((symbol >= 'A') && (symbol <= 'Z')) {
        return 0;
    }
    return 1;
}

int register_user() {
    if (user_count >= MAX_USERS) {
        printf("User limit reached. Cannot register more users.\n");
        return ERROR_USERS_LIMIT;
    }

    char login[MAX_LOGIN_LEN];
    int pin;

    printf("Enter login (max 6 characters, alphanumeric): ");
    scanf("%6s", login);
    if (find_user(login) != -1) {
        printf("User already exists.\n");
        return ERROR_USER_ALREADY_EXIST;
    }
    int len = strlen(login);
    for (int i = 0; i < len; i++) {
        if (is_trash(login[i])) {
            printf("In login must be only digits and liters");
            return ERROR_TRASH_IN_LOGIN;
        }
    }

    printf("Enter PIN (0 to 100000): ");
    scanf("%d", &pin);
    if (pin < 0 || pin > 100000) {
        printf("Invalid PIN. Must be between 0 and 100000.\n");
        return ERROR_INVALID_PIN;
    }

    strcpy(users[user_count].login, login);
    users[user_count].pin = pin;
    users[user_count].request_limit = -1;
    users[user_count].request_count = 0;
    user_count++;
    printf("User registered successfully.\n");
    return SUCCESS;
}

int login_user() {
    char login[MAX_LOGIN_LEN];
    int pin;

    printf("Enter login: ");
    scanf("%6s", login);
    int user_index = find_user(login);
    if (user_index == -1) {
        printf("User not found.\n");
        return ERROR_NO_USER;
    }

    printf("Enter PIN: ");
    scanf("%d", &pin);
    if (users[user_index].pin != pin) {
        printf("Invalid PIN.\n");
        return ERROR_INVALID_PIN;
    }

    return user_index;
}

void display_time() {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    printf("Current time: %02d:%02d:%02d\n", t->tm_hour, t->tm_min, t->tm_sec);
}

void display_date() {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    printf("Current date: %02d-%02d-%04d\n", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);
}

long long howmuch(time_t start_time, const char* flag) {
    time_t current_time;
    time(&current_time);

    double diff = difftime(current_time, start_time);

    if (strcmp(flag, "-s") == 0) {
        return (long long)diff;
    } else if (strcmp(flag, "-m") == 0) {
        return (long long)(diff / 60);
    } else if (strcmp(flag, "-h") == 0) {
        return (long long)(diff / 3600);
    } else if (strcmp(flag, "-y") == 0) {
        return (long long)(diff / (3600 * 24 * 365));
    } else {
        printf("Invalid flag.\n");
        return ERROR_INVALID_FLAG;
    }
}

int parse_input(char* input, struct tm* start_date, char* flag) {
    int year, month, day, hour, minute, second;
    if (sscanf(input, "%d-%d-%d %d:%d:%d %s", &year, &month, &day, &hour, &minute, &second, flag) != 7) {
        return ERROR_TIME_PARSING;
    }

    start_date->tm_year = year - 1900;
    start_date->tm_mon = month - 1;
    start_date->tm_mday = day;
    start_date->tm_hour = hour; 
    start_date->tm_min = minute;   
    start_date->tm_sec = second;

    return SUCCESS;
}

void apply_sanctions(User* user, const char *username, int limit) {
    int index = find_user(username);
    
    if (index == -1) {
        printf("User not found.\n");
        return;
    }

    if (limit <= 0) {
        printf("Limit must be greater than zero.\n");
        return;
    }

    printf("Enter confirmation number: ");
    int confirmation;
    scanf("%d", &confirmation);
    
    if (confirmation != 12345) {
        printf("Invalid confirmation number.\n");
        return;
    }

    users[index].request_limit = limit;
}

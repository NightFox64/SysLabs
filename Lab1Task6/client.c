#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/limits.h>

#define FIFO_SERVER "server_fifo"
#define FIFO_CLIENT "client_fifo"
#define BUFFER_SIZE 4096

int main() {
    printf("Client started. Enter absolute file paths (one per line, empty line to finish):\n");

    char input[BUFFER_SIZE] = {0};
    char line[PATH_MAX];

    while (1) {
        fgets(line, PATH_MAX, stdin);
        if (line[0] == '\n') break;
        strcat(input, line);
    }

    int server_fd = open(FIFO_SERVER, O_WRONLY);
    write(server_fd, input, strlen(input) + 1);
    close(server_fd);

    int client_fd = open(FIFO_CLIENT, O_RDONLY);
    char result[BUFFER_SIZE * 10] = {0};
    read(client_fd, result, BUFFER_SIZE * 10);
    close(client_fd);

    printf("\nServer response:\n%s\n", result);

    return 0;
}
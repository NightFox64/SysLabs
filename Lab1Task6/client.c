#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define FIFO_SERVER "server_fifo"
#define FIFO_CLIENT "client_fifo"

int main(int argc, char *argv[]) {
    int server_fd, client_fd;
    char buffer[BUFSIZ];
    char input[BUFSIZ];

    if (argc < 2) {
        printf("Usage: %s <absolute_path1> [absolute_path2 ...]\n", argv[0]);
        return 1;
    }

    input[0] = '\0';
    for (int i = 1; i < argc; i++) {
        strncat(input, argv[i], BUFSIZ - strlen(input) - 1);
        if (i != argc - 1) {
            strncat(input, "\n", BUFSIZ - strlen(input) - 1);
        }
    }

    server_fd = open(FIFO_SERVER, O_WRONLY);
    if (server_fd == -1) {
        printf("open server_fifo");
        return 1;
    }

    write(server_fd, input, strlen(input) + 1);
    close(server_fd);

    client_fd = open(FIFO_CLIENT, O_RDONLY);
    if (client_fd == -1) {
        printf("open client_fifo");
        return 1;
    }

    ssize_t bytes_read = read(client_fd, buffer, BUFSIZ - 1);
    if (bytes_read == -1) {
        printf("read");
        close(client_fd);
        return 1;
    }
    buffer[bytes_read] = '\0';

    printf("Server response:\n%s\n", buffer);
    close(client_fd);

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

#define FIFO_SERVER "server_fifo"
#define FIFO_CLIENT "client_fifo"

void process_path(const char *path, char *result, size_t result_size) {
    DIR *dir;
    struct dirent *entry;
    char dir_path[BUFSIZ];
    char *last_slash;

    strncpy(dir_path, path, BUFSIZ - 1);
    last_slash = strrchr(dir_path, '/');
    if (last_slash == NULL) {
        snprintf(result, result_size, "Invalid path: %s\n", path);
        return;
    }
    *last_slash = '\0';

    dir = opendir(dir_path);
    if (dir == NULL) {
        snprintf(result, result_size, "Cannot open directory: %s\n", dir_path);
        return;
    }

    snprintf(result, result_size, "Directory: %s\nFiles:\n", dir_path);
    size_t len = strlen(result);

    while ((entry = readdir(dir)) != NULL && len < result_size - 256) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        len += snprintf(result + len, result_size - len, "- %s\n", entry->d_name);
    }

    closedir(dir);
}

int main() {
    int server_fd, client_fd;
    char buffer[BUFSIZ];
    char result[BUFSIZ * 4];
    ssize_t bytes_read;

    mkfifo(FIFO_SERVER, 0666);
    mkfifo(FIFO_CLIENT, 0666);

    printf("Server started. Waiting for clients... (write 'end' to shut down the server)\n");

    while (1) {
        server_fd = open(FIFO_SERVER, O_RDONLY);
        if (server_fd == -1) {
            printf("open server_fifo");
            break;
        }

        bytes_read = read(server_fd, buffer, BUFSIZ - 1);
        if (bytes_read == -1) {
            printf("read");
            close(server_fd);
            continue;
        }
        buffer[bytes_read] = '\0';
        close(server_fd);

        printf("Received paths: %s\n", buffer);

        result[0] = '\0';
        char *path = strtok(buffer, "\n");

        while (path != NULL) {
            char temp_result[BUFSIZ * 2];
            process_path(path, temp_result, sizeof(temp_result));
            strncat(result, temp_result, sizeof(result) - strlen(result) - 1);
            strncat(result, "\n", sizeof(result) - strlen(result) - 1);
            path = strtok(NULL, "\n");
        }

        client_fd = open(FIFO_CLIENT, O_WRONLY);
        if (client_fd == -1) {
            printf("open client_fifo");
            continue;
        }

        write(client_fd, result, strlen(result) + 1);
        close(client_fd);
    }

    unlink(FIFO_SERVER);
    unlink(FIFO_CLIENT);
    return 0;
}
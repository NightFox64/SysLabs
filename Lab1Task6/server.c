#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/limits.h>

#define FIFO_SERVER "server_fifo"
#define FIFO_CLIENT "client_fifo"
#define BUFFER_SIZE 4096
#define DT_REG 8

typedef struct {
    char dir[PATH_MAX];
    char files[BUFFER_SIZE];
} DirEntry;

int is_directory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) return 0;
    return S_ISDIR(statbuf.st_mode);
}

void get_files_in_dir(const char *dir_path, char *files_list) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("opendir failed");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            strcat(files_list, entry->d_name);
            strcat(files_list, "\n");
        }
    }
    closedir(dir);
}

void process_paths(const char *input, char *output) {
    char *paths = strdup(input);
    char *path = strtok(paths, "\n");
    DirEntry entries[100];
    int entry_count = 0;

    while (path != NULL) {
        if (path[0] != '/') {
            path = strtok(NULL, "\n");
            continue;
        }

        char dir_path[PATH_MAX];
        strcpy(dir_path, path);
        char *last_slash = strrchr(dir_path, '/');
        if (last_slash != NULL) {
            *last_slash = '\0';
        }

        if (!is_directory(dir_path)) {
            path = strtok(NULL, "\n");
            continue;
        }

        int found = 0;
        for (int i = 0; i < entry_count; i++) {
            if (strcmp(entries[i].dir, dir_path) == 0) {
                found = 1;
                break;
            }
        }

        if (!found && entry_count < 100) {
            strcpy(entries[entry_count].dir, dir_path);
            entries[entry_count].files[0] = '\0';
            get_files_in_dir(dir_path, entries[entry_count].files);
            entry_count++;
        }

        path = strtok(NULL, "\n");
    }

    free(paths);

    output[0] = '\0';
    for (int i = 0; i < entry_count; i++) {
        strcat(output, "Directory: ");
        strcat(output, entries[i].dir);
        strcat(output, "\nFiles:\n");
        strcat(output, entries[i].files);
        strcat(output, "\n");
    }
}

int main() {
    mkfifo(FIFO_SERVER, 0666);
    mkfifo(FIFO_CLIENT, 0666);

    printf("Server started. Waiting for client messages...\n");

    int server_fd = open(FIFO_SERVER, O_RDONLY);
    int client_fd = open(FIFO_CLIENT, O_WRONLY);

    char buffer[BUFFER_SIZE];
    read(server_fd, buffer, BUFFER_SIZE);

    printf("Received paths:\n%s\n", buffer);

    char result[BUFFER_SIZE * 10] = {0};
    process_paths(buffer, result);

    write(client_fd, result, strlen(result) + 1);

    close(server_fd);
    close(client_fd);
    unlink(FIFO_SERVER);
    unlink(FIFO_CLIENT);

    return 0;
}
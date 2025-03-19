#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdint.h>
#include <errno.h>

#define BLOCK_SIZE 1024

// Функция для выполнения операции xorN
void xorN(const char *filename, int n) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    uint8_t buffer[BLOCK_SIZE];
    uint8_t xor_result = 0;
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, BLOCK_SIZE, file)) > 0) {
        for (size_t i = 0; i < bytes_read; i++) {
            xor_result ^= buffer[i];
        }
    }

    fclose(file);

    printf("XOR result for %s (N=%d): %02x\n", filename, n, xor_result);
}

// Функция для выполнения операции mask
void mask(const char *filename, uint32_t mask_value) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    uint32_t value;
    size_t count = 0;

    while (fread(&value, sizeof(uint32_t), 1, file)) {
        if ((value & mask_value) == mask_value) {
            count++;
        }
    }

    fclose(file);

    printf("Mask count for %s: %zu\n", filename, count);
}

// Функция для выполнения операции copyN
void copyN(const char *filename, int n) {
    for (int i = 1; i <= n; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            char new_filename[256];
            snprintf(new_filename, sizeof(new_filename), "%s_%d", filename, i);

            FILE *src_file = fopen(filename, "rb");
            if (!src_file) {
                perror("Failed to open source file");
                exit(EXIT_FAILURE);
            }

            FILE *dst_file = fopen(new_filename, "wb");
            if (!dst_file) {
                perror("Failed to open destination file");
                fclose(src_file);
                exit(EXIT_FAILURE);
            }

            uint8_t buffer[BLOCK_SIZE];
            size_t bytes_read;

            while ((bytes_read = fread(buffer, 1, BLOCK_SIZE, src_file))) {
                fwrite(buffer, 1, bytes_read, dst_file);
            }

            fclose(src_file);
            fclose(dst_file);

            exit(EXIT_SUCCESS);
        } else if (pid < 0) {
            perror("Failed to fork");
            return;
        }
    }

    for (int i = 0; i < n; i++) {
        wait(NULL);
    }
}

// Функция для выполнения операции find
void find(const char *filename, const char *search_string) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    char line[BLOCK_SIZE];
    int found = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, search_string)) {
            found = 1;
            break;
        }
    }

    fclose(file);

    if (found) {
        printf("String found in %s\n", filename);
    } else {
        printf("String not found in %s\n", filename);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <file1> <file2> ... <flag> [args]\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *flag = argv[argc - 1];
    int file_count = argc - 2;

    if (strncmp(flag, "xor", 3) == 0) {
        int n = atoi(flag + 3);
        if (n < 2 || n > 6) {
            fprintf(stderr, "Invalid N value for xorN\n");
            return EXIT_FAILURE;
        }

        for (int i = 1; i <= file_count; i++) {
            xorN(argv[i], n);
        }
    } else if (strncmp(flag, "mask", 4) == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s <file1> <file2> ... mask <hex>\n", argv[0]);
            return EXIT_FAILURE;
        }

        uint32_t mask_value = strtoul(argv[argc - 1], NULL, 16);

        for (int i = 1; i <= file_count; i++) {
            mask(argv[i], mask_value);
        }
    } else if (strncmp(flag, "copy", 4) == 0) {
        int n = atoi(flag + 4);
        if (n <= 0) {
            fprintf(stderr, "Invalid N value for copyN\n");
            return EXIT_FAILURE;
        }

        for (int i = 1; i <= file_count; i++) {
            copyN(argv[i], n);
        }
    } else if (strncmp(flag, "find", 4) == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s <file1> <file2> ... find <string>\n", argv[0]);
            return EXIT_FAILURE;
        }

        char *search_string = argv[argc - 1];

        for (int i = 1; i <= file_count; i++) {
            pid_t pid = fork();
            if (pid == 0) {
                find(argv[i], search_string);
                exit(EXIT_SUCCESS);
            } else if (pid < 0) {
                perror("Failed to fork");
                return EXIT_FAILURE;
            }
        }

        for (int i = 0; i < file_count; i++) {
            wait(NULL);
        }
    } else {
        fprintf(stderr, "Unknown flag: %s\n", flag);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
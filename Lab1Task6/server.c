#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

#define SHM_SIZE 1024
#define MAX_PATHS 100
#define MAX_PATH_LEN 256
#define MAX_FILES 100
#define DT_REG 8

// Структура для хранения информации о каталоге и его файлах
typedef struct {
    char dir_path[MAX_PATH_LEN];
    char files[MAX_FILES][MAX_PATH_LEN];
    int file_count;
} DirInfo;

// Структура для передачи данных через разделяемую память
typedef struct {
    int path_count;
    char paths[MAX_PATHS][MAX_PATH_LEN];
    DirInfo dirs[MAX_PATHS];
    int dir_count;
} SharedData;

// Операции с семафором
void sem_lock(int sem_id) {
    struct sembuf sb = {0, -1, 0};
    semop(sem_id, &sb, 1);
}

void sem_unlock(int sem_id) {
    struct sembuf sb = {0, 1, 0};
    semop(sem_id, &sb, 1);
}

// Получение информации о файлах в каталогах
void process_directories(SharedData *data) {
    data->dir_count = 0;

    for (int i = 0; i < data->path_count; i++) {
        char *path = data->paths[i];
        struct stat statbuf;

        if (stat(path, &statbuf)) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            // Это каталог, добавляем его в список
            strncpy(data->dirs[data->dir_count].dir_path, path, MAX_PATH_LEN);
            data->dirs[data->dir_count].file_count = 0;

            DIR *dir = opendir(path);
            if (!dir) {
                perror("opendir");
                continue;
            }

            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_type == DT_REG) { // Обычный файл
                    if (data->dirs[data->dir_count].file_count < MAX_FILES) {
                        strncpy(data->dirs[data->dir_count].files[data->dirs[data->dir_count].file_count],
                                entry->d_name, MAX_PATH_LEN);
                        data->dirs[data->dir_count].file_count++;
                    }
                }
            }
            closedir(dir);
            data->dir_count++;
        }
    }
}

int main() {
    key_t key = ftok(".", 'S');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // Создаем разделяемую память
    int shm_id = shmget(key, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget");
        exit(1);
    }

    // Подключаем разделяемую память
    SharedData *data = (SharedData *)shmat(shm_id, NULL, 0);
    if (data == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    // Создаем семафор
    int sem_id = semget(key, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("semget");
        exit(1);
    }

    // Инициализируем семафор значением 1 (разблокирован)
    semctl(sem_id, 0, SETVAL, 1);

    printf("Сервер запущен. Ожидание данных от клиента...\n");

    while (1) {
        sem_lock(sem_id);

        if (data->path_count > 0) {
            printf("Получены пути от клиента. Обработка...\n");

            process_directories(data);

            printf("Обработка завершена. Ожидание нового запроса...\n");
            data->path_count = 0; // Сбрасываем счетчик для следующего запроса
        }

        sem_unlock(sem_id);
        sleep(1); // Чтобы не нагружать CPU
    }

    // Отключаем разделяемую память (не будет выполнено из-за бесконечного цикла)
    shmdt(data);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);

    return 0;
}
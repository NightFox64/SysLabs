#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define SHM_SIZE 1024
#define MAX_PATHS 100
#define MAX_PATH_LEN 256
#define MAX_FILES 100

typedef struct {
    char dir_path[MAX_PATH_LEN];
    char files[MAX_FILES][MAX_PATH_LEN];
    int file_count;
} DirInfo;

typedef struct {
    int path_count;
    char paths[MAX_PATHS][MAX_PATH_LEN];
    DirInfo dirs[MAX_PATHS];
    int dir_count;
} SharedData;

void sem_lock(int sem_id) {
    struct sembuf sb = {0, -1, 0};
    semop(sem_id, &sb, 1);
}

void sem_unlock(int sem_id) {
    struct sembuf sb = {0, 1, 0};
    semop(sem_id, &sb, 1);
}

int main() {
    key_t key = ftok(".", 'S');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // Подключаемся к разделяемой памяти
    int shm_id = shmget(key, sizeof(SharedData), 0666);
    if (shm_id == -1) {
        perror("shmget");
        exit(1);
    }

    SharedData *data = (SharedData *)shmat(shm_id, NULL, 0);
    if (data == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    // Подключаемся к семафору
    int sem_id = semget(key, 1, 0666);
    if (sem_id == -1) {
        perror("semget");
        exit(1);
    }

    // Вводим пути от пользователя
    printf("Введите абсолютные пути к файлам/каталогам (по одному в строке, 'end' для завершения):\n");

    char input[MAX_PATH_LEN];
    data->path_count = 0;

    while (1) {
        fgets(input, MAX_PATH_LEN, stdin);
        input[strcspn(input, "\n")] = '\0'; // Удаляем символ новой строки

        if (strcmp(input, "end") == 0) {
            break;
        }

        if (data->path_count < MAX_PATHS) {
            strncpy(data->paths[data->path_count], input, MAX_PATH_LEN);
            data->path_count++;
        } else {
            printf("Достигнуто максимальное количество путей.\n");
            break;
        }
    }

    // Отправляем данные серверу
    sem_lock(sem_id);
    printf("Отправка данных серверу...\n");
    sem_unlock(sem_id);

    // Ждем, пока сервер обработает данные
    sleep(2);

    // Читаем результаты
    sem_lock(sem_id);
    printf("\nРезультаты:\n");

    // Здесь мы должны привести data к полной структуре, как на сервере
    // В реальном приложении лучше использовать одну и ту же структуру в обоих программах
    // Для простоты выведем только пути каталогов (в реальности нужно добавить файлы)
    for (int i = 0; i < data->dir_count; i++) {
        printf("Каталог: %s\n", ((DirInfo *)(data->dirs))[i].dir_path);
        printf("Файлы:\n");
        for (int j = 0; j < ((DirInfo *)(data->dirs))[i].file_count; j++) {
            printf("  %s\n", ((DirInfo *)(data->dirs))[i].files[j]);
        }
        printf("\n");
    }

    sem_unlock(sem_id);

    // Отключаемся от разделяемой памяти
    shmdt(data);

    return 0;
}
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define N 5

sem_t forks[N];
sem_t waiter;

void *philosopher_unsync(void *num) {
    int id = *(int *)num;
    int left = id;
    int right = (id + 1) % N;

    while (1) {
        printf("Philosoph %d thinks\n", id);
        sleep(1);

        printf("Philosoph %d wanna take forks %d and %d\n", id, left, right);
        
        printf("Philosoph %d took fork %d\n", id, left);
        
        sleep(1);
        
        printf("Philosoph %d took fork %d\n", id, right);
        
        printf("Philosoph %d eats\n", id);
        sleep(1);
        
        printf("Philosoph %d put forks %d and %d\n", id, left, right);
    }
}

void *philosopher_sync(void *num) {
    int id = *(int *)num;
    int left = id;
    int right = (id + 1) % N;

    while (1) {
        printf("Philososh %d thinks\n", id);
        sleep(1);

        printf("Philosoph %d wanna take forks %d and %d\n", id, left, right);
        
        sem_wait(&waiter);
        
        sem_wait(&forks[left]);
        printf("Philosoph %d took fork %d\n", id, left);
        sem_wait(&forks[right]);
        printf("Philosoph %d took fork %d\n", id, right);
        
        sem_post(&waiter);
        
        printf("Philosoph %d eats\n", id);
        sleep(1);
        
        sem_post(&forks[left]);
        sem_post(&forks[right]);
        printf("Philosoph %d put forks %d и %d\n", id, left, right);
    }
}

int main() {
    pthread_t philosophers[N];
    int ids[N];
    
    printf("No sync for next 5 sec:\n");
    
    for (int i = 0; i < N; i++) {
        ids[i] = i;
        pthread_create(&philosophers[i], NULL, philosopher_unsync, &ids[i]);
    }
    
    sleep(5);
    
    for (int i = 0; i < N; i++) {
        pthread_cancel(philosophers[i]);
    }
    
    printf("\nYes sync for next 10 sec:\n");
    
    for (int i = 0; i < N; i++) {
        sem_init(&forks[i], 0, 1);
    }
    sem_init(&waiter, 0, N - 1);
    
    for (int i = 0; i < N; i++) {
        ids[i] = i;
        pthread_create(&philosophers[i], NULL, philosopher_sync, &ids[i]);
    }
    
    sleep(10);
    
    for (int i = 0; i < N; i++) {
        pthread_cancel(philosophers[i]);
    }
    
    for (int i = 0; i < N; i++) {
        sem_destroy(&forks[i]);
    }
    sem_destroy(&waiter);
    
    return 0;
}
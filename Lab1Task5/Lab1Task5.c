#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef enum {
    EMPTY,
    WOMEN_ONLY,
    MEN_ONLY
} bathroom_state;

typedef struct {
    bathroom_state state;
    int count;
    int max_people;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} bathroom_t;

bathroom_t bathroom;

void bathroom_init(int max_people) {
    bathroom.state = EMPTY;
    bathroom.count = 0;
    bathroom.max_people = max_people;
    pthread_mutex_init(&bathroom.mutex, NULL);
    pthread_cond_init(&bathroom.cond, NULL);
}

void bathroom_destroy() {
    pthread_mutex_destroy(&bathroom.mutex);
    pthread_cond_destroy(&bathroom.cond);
}

void woman_wants_to_enter() {
    pthread_mutex_lock(&bathroom.mutex);
    
    while ((bathroom.state == MEN_ONLY) || 
           (bathroom.count >= bathroom.max_people)) {
        pthread_cond_wait(&bathroom.cond, &bathroom.mutex);
    }
    
    if (bathroom.state == EMPTY) {
        bathroom.state = WOMEN_ONLY;
    }
    
    bathroom.count++;
    printf("Woman entered. Now in bathroom: %d women\n", bathroom.count);
    
    pthread_mutex_unlock(&bathroom.mutex);
}

void man_wants_to_enter() {
    pthread_mutex_lock(&bathroom.mutex);
    
    while ((bathroom.state == WOMEN_ONLY) || 
           (bathroom.count >= bathroom.max_people)) {
        pthread_cond_wait(&bathroom.cond, &bathroom.mutex);
    }
    
    if (bathroom.state == EMPTY) {
        bathroom.state = MEN_ONLY;
    }
    
    bathroom.count++;
    printf("Man entered. Now in bathhroom: %d men\n", bathroom.count);
    
    pthread_mutex_unlock(&bathroom.mutex);
}

void woman_leaves() {
    pthread_mutex_lock(&bathroom.mutex);
    
    bathroom.count--;
    printf("Woman left. Remained in bathroom: %d women\n", bathroom.count);
    
    if (bathroom.count == 0) {
        bathroom.state = EMPTY;
    }
    
    pthread_cond_broadcast(&bathroom.cond);
    pthread_mutex_unlock(&bathroom.mutex);
}

void man_leaves() {
    pthread_mutex_lock(&bathroom.mutex);
    
    bathroom.count--;
    printf("Mane left. Remained in bathroom: %d men\n", bathroom.count);
    
    if (bathroom.count == 0) {
        bathroom.state = EMPTY;
    }
    
    pthread_cond_broadcast(&bathroom.cond);
    pthread_mutex_unlock(&bathroom.mutex);
}

void* woman_thread(void* arg) {
    woman_wants_to_enter();
    sleep(1);
    woman_leaves();
    return NULL;
}

void* man_thread(void* arg) {
    man_wants_to_enter();
    sleep(1);
    man_leaves();
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <N>\n", argv[0]);
        return 1;
    }
    
    int N = atoi(argv[1]);
    if (N <= 0) {
        printf("N must be positive\n");
        return 1;
    }
    
    bathroom_init(N);
    
    pthread_t threads[10];
    
    pthread_create(&threads[0], NULL, woman_thread, NULL);
    pthread_create(&threads[1], NULL, man_thread, NULL);
    pthread_create(&threads[2], NULL, woman_thread, NULL);
    pthread_create(&threads[3], NULL, man_thread, NULL);
    pthread_create(&threads[4], NULL, woman_thread, NULL);
    pthread_create(&threads[5], NULL, man_thread, NULL);
    pthread_create(&threads[6], NULL, woman_thread, NULL);
    pthread_create(&threads[7], NULL, man_thread, NULL);
    pthread_create(&threads[8], NULL, woman_thread, NULL);
    pthread_create(&threads[9], NULL, man_thread, NULL);
    
    for (int i = 0; i < 10; i++) {
        pthread_join(threads[i], NULL);
    }
    
    bathroom_destroy();
    
    return 0;
}
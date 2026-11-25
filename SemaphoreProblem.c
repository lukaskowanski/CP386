#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int counter = 0;
sem_t sem;

int *increment(void *arg) {
    for (int i = 0; i < 100000; i++) {
        sem_wait(&sem);
        counter++;
        sem_post(&sem);
    }

    return NULL;
}

int main() {
    pthread_t t1, t2, t3;

    sem_init(&sem, 0, 1);

    pthread_create(&t1, NULL, increment, NULL);
    pthread_create(&t2, NULL, increment, NULL);
    pthread_create(&t3, NULL, increment, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    printf("Final counter value: %d\n", counter);

    sem_destroy(&sem);
    return 0;
}
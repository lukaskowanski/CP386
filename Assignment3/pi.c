// Monte Carlo Problem
// Libraries
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// Global Constants
#define RAND_MAX 50000
#define NUM_THREADS 4

double INSIDE_CIRCLE = 0;

// Helper function
double random_double() {
    return random() / ((double)RAND_MAX + 1);
}

// Thread function
void* ThreadFunc(void* arg) {
    // Each thread will generate x amount of random points
    for (int j = 0; j < RAND_MAX / NUM_THREADS; j++) {
        double x = random_double() * 2.0 - 1.0;
        double y = random_double() * 2.0 - 1.0;

        // Check to see if random point is inside circle
        if (x * x + y * y <= 1.0) {
            INSIDE_CIRCLE++;
        }
    }

    printf("Hello from thread! Points inside circle: %f\n", INSIDE_CIRCLE);
    return NULL;
}

int mainThreads(int num_threads) {
    pthread_t threads[num_threads];

    // Create new threads
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, ThreadFunc, NULL);
    }

    // Wait for threads to finish
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}

int main() {

    mainThreads(NUM_THREADS);

    printf("Total points inside circle: %f\n", INSIDE_CIRCLE);

    return 0;
}
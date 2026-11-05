// Monte Carlo pi Problem
// Libraries
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// Count struct
typedef struct {
    int inside_circle;
    int total_points;
} Count;

// Helper function
double randomDouble() {
    /* Generate a random double between 0 and 1 */
    return random() / ((double)RAND_MAX + 1);
}

// Thread function
void* threadFunc(void* arg) {
    /* Thread function generates random points and counts those inside the circle */
    Count* my_count = (Count*)arg;

    // Each thread will generate specified number of random points
    for (int j = 0; j < my_count->total_points; j++) {
        double x = randomDouble() * 2.0 - 1.0;
        double y = randomDouble() * 2.0 - 1.0;

        // Check to see if random points are inside circle
        if (x * x + y * y <= 1.0) {
            my_count->inside_circle++;
        }
    }

    pthread_exit(NULL);
}

double mainThread(int max_points, int num_threads) {
    /* Main thread function that creates and manages threads */ 
    pthread_t threads[num_threads - 1];

    Count *counts = malloc((num_threads - 1) * sizeof(Count));
    if (counts == NULL) {
        perror("Failed to allocate memory for counts");
        exit(1);
    }

    // Create new threads using Count structs
    for (int i = 0; i < num_threads - 1; i++) {
        counts[i].inside_circle = 0;
        counts[i].total_points = max_points / (num_threads - 1);

        int result = pthread_create(&threads[i], NULL, threadFunc, &counts[i]);
        if (result) {
            perror("Failed to create thread");
            exit(1);
        }
    }
    
    // Wait for threads to finish...
    for (int i = 0; i < num_threads - 1; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join thread");
            exit(1);
        }
    }

    // Sums up results from all threads
    double total_hits = 0.0;
    for (int i = 0; i < num_threads - 1; i++) {
        total_hits += counts[i].inside_circle;
    }
    double final_pi = total_hits / max_points * 4.0;

    // Freeing the allocated memory for counts
    free(counts);

    return final_pi;
}

int main(int argc, char* argv[]) {
    // Error handling for command line arguments
    if (argc < 3 || argv[1] == NULL || argv[2] == NULL) {
        perror("usage: pi <integer value for NUMBER_OF_DARTS> <integer value for NUM_THREADS>");
        exit(1);
    }

    int NUMBER_OF_DARTS = strtol(argv[1], NULL, 10);
    if (NUMBER_OF_DARTS < 5000000) {
        perror("The number of darts must be >= 5000000");
        exit(1);
    }

    int NUM_THREADS = strtol(argv[2], NULL, 10);
    if (NUM_THREADS < 2) {
        perror("The number of threads must be >= 2");
        exit(1);
    }

    double pi = mainThread(NUMBER_OF_DARTS, NUM_THREADS);
    
    printf("Pi = %f\n", pi);

    return 0;
}
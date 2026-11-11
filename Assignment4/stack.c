// Updated Solution to Example Given - Stack (mutex locks)
// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 10

pthread_mutex_t lock; // init mutex lock

typedef struct Node {
    int data;
    struct Node *next;
} StackNode;
StackNode *top;

// function prototypes
void push(int v, StackNode **top);
int pop(StackNode **top);
int is_empty(StackNode *top);

// push function
void push(int v, StackNode **top) {
    StackNode *new_node;
    printf("Thread is running Push() operation for value: %d\n", v);

    // Critical section ??
    pthread_mutex_lock(&lock);
    new_node = (StackNode *)malloc(sizeof(StackNode));
    new_node->data = v;
    new_node->next = *top;
    *top = new_node;
    pthread_mutex_unlock(&lock);

}

// pop function
int pop(StackNode **top) {
	StackNode *temp;
    
    if (is_empty(*top)) {
        printf("Stack empty \n");
        return 0;
	} else {
        // Critical Section ??
        pthread_mutex_lock(&lock);
        int data = (*top)->data;
        printf("Thread is running Pop() operation and value is: %d\n",data);
		temp = *top;
        *top = (*top)->next;
        pthread_mutex_unlock(&lock);

		free(temp);
        return data;
    }
}

// Check if top is NULL
int is_empty(StackNode *top) {
    if (top == NULL)
        return 1;
    else
        return 0;
}

// Thread's push function
void* thread_push(void *args) {
    int *threadId = (int *)args;
    push(*threadId + 1, &top);
    return NULL;
}

// Thread's pop function 
void* thread_pop() {
    pop(&top);
    return NULL;
}

// main function 
int main(void) {
    pthread_t threads_push[NUM_THREADS];
    pthread_t threads_pop[NUM_THREADS];
    int thread_args[NUM_THREADS];
    int i, j;

    // Creating push threads
    for(i = 0; i < NUM_THREADS; i++) {
	    thread_args[i] = i;
	    pthread_create(&threads_push[i], NULL, thread_push, (void *)&thread_args[i]);
	}

    // Create Pop Threads
	for(i = 0; i < NUM_THREADS; i++) {
        thread_args[i] = i;
	    pthread_create(&threads_pop[i], NULL, thread_pop, NULL);
	}

    // Join Push Threads
	for(j = 0; j < NUM_THREADS; j++) {
	pthread_join(threads_push[j], NULL);
	}

    // Join Pop Threads
	for(j = 0; j < NUM_THREADS; j++) {
	pthread_join(threads_pop[j], NULL);
	}

    pthread_exit(NULL);
	return 0;
}

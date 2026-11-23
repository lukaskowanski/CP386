// Updated Solution to Given Example - Thread Synchronization (semaphores)
// Libraries
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <semaphore.h>

sem_t running;

void logStart(char *tID); // function to log that a new thread is started
void logFinish(char *tID); // function to log that a thread has finished its time

void startClock(); // function to start program clock
long getCurrentTime(); // function to check current time since clock was started
time_t programClock; // the global timer/clock for the program

typedef struct thread { // represents a single thread, you can add more members if required
	char tid[4]; // id of the thread as read from file
	unsigned int startTime;
	int state;
	pthread_t handle;
	int retVal;
	sem_t crit; // semaphore for critical section
} Thread;

// you can add more functions here if required

int threadsLeft(Thread *threads, int threadCount);
int threadToStart(Thread *threads, int threadCount);
int threadToRun(Thread *threads, int threadCount, int previousParity);
void* threadRun(void *t); // the thread function, the code executed by each thread
int readFile(char *fileName, Thread **threads); // function to read the file content and build array of threads

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Input file name missing...exiting with error code -1\n");
		return -1;
	}

	// you can add some suitable code anywhere in main() if required

	Thread *threads = NULL;
	int threadCount = readFile(argv[1], &threads);

	int previousParity = -1; 

	sem_init(&running,  0, 1);
	for (int k = 0; k < threadCount; k++) {
		sem_init(&threads[k].crit, 0, 0); // Initialize critical section semaphore for each thread
	}

	startClock();
	while (threadsLeft(threads, threadCount) > 0) { // put a suitable condition here to run your program
		// you can add some suitable code anywhere in this loop if required

		int i = 0;
		while ((i = threadToStart(threads, threadCount)) > -1) {
			// you can add some suitable code anywhere in this loop if required

			threads[i].state = 1;
			threads[i].retVal = pthread_create(&(threads[i].handle), NULL,
					threadRun, &threads[i]);

			if (threads[i].retVal != 0) {
				printf("Error in creating thread %s...exiting with error code -1\n",
						threads[i].tid);
				return -1; // returning -1
			}
		}

		if (sem_trywait(&running) == 0) {
			i = threadToRun(threads, threadCount, previousParity);

			if (i > -1) {
				previousParity = atoi(threads[i].tid + 1) % 2;
				sem_post(&threads[i].crit); // Allow the thread to enter its critical section
			} else {
				sem_post(&running); 
			}
		}
	}

	for (int k = 0; k < threadCount; k++) {
		pthread_join(threads[k].handle, NULL);
		sem_destroy(&threads[k].crit); 
	}

	free(threads); // Freeing the allocated memory for threads

	sem_destroy(&running);

	return 0;
}

int readFile(char *fileName, Thread **threads) { // do not modify this method
	FILE *in = fopen(fileName, "r");
	if (!in) {
		printf(
				"Child A: Error in opening input file...exiting with error code -1\n");
		return -1;
	}

	struct stat st;
	fstat(fileno(in), &st);
	char *fileContent = (char*) malloc(((int) st.st_size + 1) * sizeof(char));
	fileContent[0] = '\0';
	while (!feof(in)) {
		char line[100];
		if (fgets(line, 100, in) != NULL) {
			strncat(fileContent, line, strlen(line));
		}
	}
	fclose(in);

	char *command = NULL;
	int threadCount = 0;
	char *fileCopy = (char*) malloc((strlen(fileContent) + 1) * sizeof(char));
	strcpy(fileCopy, fileContent);
	command = strtok(fileCopy, "\r\n");
	
    while (command != NULL) {
		threadCount++;
		command = strtok(NULL, "\r\n");
	}
	*threads = (Thread*) malloc(sizeof(Thread) * threadCount);

	char *lines[threadCount];
	command = NULL;
	int i = 0;
	command = strtok(fileContent, "\r\n");

	while (command != NULL) {
		lines[i] = malloc(sizeof(command) * sizeof(char));
		strcpy(lines[i], command);
		i++;
		command = strtok(NULL, "\r\n");
	}

	for (int k = 0; k < threadCount; k++) {
		char *token = NULL;
		int j = 0;
		token = strtok(lines[k], ";");
		while (token != NULL) {
			//if you have extended the Thread struct then here
			//you can do initialization of those additional members
			//or any other action on the Thread members
			(*threads)[k].state = 0;
			if (j == 0)
				strcpy((*threads)[k].tid, token);
			if (j == 1)
				(*threads)[k].startTime = atoi(token);
			j++;
			token = strtok(NULL, ";");
		}
	}
	return threadCount;
}

void logStart(char *tID) {
	printf("[%ld] New Thread with ID %s is started.\n", getCurrentTime(), tID);
}

void logFinish(char *tID) {
	printf("[%ld] Thread with ID %s is finished.\n", getCurrentTime(), tID);
}

int threadsLeft(Thread *threads, int threadCount) {
	int remainingThreads = 0;
	for (int k = 0; k < threadCount; k++) {
		if (threads[k].state > -1)
			remainingThreads++;
	}
	return remainingThreads;
}

int threadToStart(Thread *threads, int threadCount) {
	for (int k = 0; k < threadCount; k++) {
		if (threads[k].state == 0 && threads[k].startTime <= getCurrentTime())
			return k;
		}
		
	return -1;
}

// Added new function 
int threadToRun(Thread *threads, int threadCount, int previousParity) {
	int best = -1;
	int threadsComing = 0;

	for (int k = 0; k < threadCount; k++) {
		if (threads[k].state == 0) {
			threadsComing = 1;
		}
	}

	// Waiting thread whose parity is different from the previous one
	for (int k = 0; k < threadCount; k++) {
		int parity = atoi(threads[k].tid + 1) % 2;

		if (threads[k].state == 1 && (previousParity == -1 || parity != previousParity)) {
			if (best == -1 || threads[k].startTime < threads[best].startTime) {
				best = k;
			}
		}
	}
	if (best != -1) {
		return best;
	}

	if (threadsComing) {
		return -1;
	}

	// If nothing else is coming, let the remaining threads go (prevents starvation)
	for (int k = 0; k < threadCount; k++) {
		if (threads[k].state == 1 && (best == -1 || strcmp(threads[k].tid, threads[best].tid) < 0)) {
			best = k;
		}
	}

	return best;
}

void* threadRun(void *t) { // implement this function in a suitable way
	logStart(((Thread*) t)->tid);

	sem_wait(&(((Thread*) t)->crit)); // Wait for permission to enter critical section
	
	// critical section starts here
	printf("[%ld] Thread %s is in its critical section\n", getCurrentTime(),
		((Thread*) t)->tid);
	// critical section ends here
	
	logFinish(((Thread*) t)->tid);
	((Thread*) t)->state = -1;
	sem_post(&running); // Signal that the thread has finished and allow another thread to run
	pthread_exit(0);
}

void startClock() {
	programClock = time(NULL);
}

long getCurrentTime() { // invoke this method whenever you want check how much time units passed
// since you invoked startClock()
	time_t now;
	now = time(NULL);
	return now - programClock;
}

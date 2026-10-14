// Process Dispatcher
// Libraries
#include <stdio.h>
#include <unistd.h>

void dispatch_process(char filename[])
{
    // Dispatches a ton of parents + children as if its trying to
    // speedrun populationmaxxing

    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork failed");
        return;
    }
    else if (pid == 0)     // Child process for reading the file
    {
        // Child process
        printf("Child process");
        printf("Child process: %d\n", pid);

        execlp("cat", "cat", filename, NULL); // Execute the cat command to read the file

        pid_t child_pid = fork(); // Child process forks to grandchild
    }
    else {
        // Parent gets file and forks to child
        wait(NULL);
    }
}

void main() {
    dispatch_process("test.txt");
}
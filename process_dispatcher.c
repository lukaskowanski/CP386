// Process Dispatcher
// Libraries
#define _GNU_SOURCE // Important for ftruncate!
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

int dispatch_process(char filename[]) {
    // Parent dispatches children to read and process a given
    // file using shared virtual memory and pipeline techniques
    // and writes the results into the "output.txt" text file

    // Initializing shared memory
    const char *shm_name = "/shared-mem";
    size_t size = 4096;

    // shared mem obj
    int fd = shm_open(shm_name, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd == -1) {
        perror("shm_open failed to open");
        close(fd);
        exit(1);
    }

    // truncates mem size for intial mem objects
    if (ftruncate(fd, size) == -1) {
        perror("ftruncate failed");
        exit(1);
    }

    // Mapping shared memory to process address space
    char *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap failed to read");
        shm_unlink(ptr);
        exit(1);
    }

    // Pipeline initialization
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(1);
    }

    // Parent forks to Manager child process
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }

    // Manager child process for reading the file
    else if (pid == 0) {
        // -- Manager child process --
        // child pipes process to parent
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        // Reader child opens file
        FILE *fp = fopen(filename, "r");
        if (fp == NULL) {
            perror("file failed to open");
            exit(1);
        }

        // Getting file size
        struct stat filestat;
        if (fstat(fp, &filestat) == -1) {
            perror("Error getting file size");
            exit(1);
        }
        size_t f_size = filestat.st_size;

        // Finally reading file
        // ssize_t bytes_read = read(fp, ptr, f_size);
        // if (bytes_read == -1) {
        //     perror("Error reading file into memory");
        //     exit(1);
        // }

        char *line = NULL;
        ssize_t read;

        while ((read = getline(&line, f_size, fp)) != -1) {
            printf("%s", line);
        }
         
        /* Read file line by line, than fork to grandchild to execute linux line. 
        Then the result of each line is written into output.txt by parent */

        // Child process forks to grandchild
        pid_t child_pid = fork();
        if (child_pid < 0) {
            perror("fork failed");
            exit(1);
        }

        else if (child_pid == 0) {
            // grandchild process

        } else {
            // continue child process
            
            wait(NULL);

            // manager child stuff
            // execlp();
            
        }

    } else {
        // -- Parent process --
        // Parent gets processed file from child via piping
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        // Parental stuff
        wait(NULL);


        exit(1);
    }

    // Cleaning up and closing shared memory
    munmap(ptr, size);
    close(fd);
    shm_unlink(shm_name);

    return 0;
}

int main() {
    dispatch_process("sample_in_dispatch.txt");

    return 0;
}

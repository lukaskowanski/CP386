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

static int writeOutput(const char *command, const char *output) {
    FILE *fp = fopen("output.txt", "a");
    if (fp == NULL) {
        perror("fopen");
        return -1;
    }
    fprintf(fp, "The output of: %s is\n", command);
    fprintf(fp, ">>>>>>>>>>>>>>>\n%s<<<<<<<<<<<<<<<\n", output);
    fclose(fp);
    return 0;

}

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
            fclose(fp);
            exit(1);
        }
        size_t f_size = filestat.st_size;

        // Clearing memory first
        munmap(ptr, f_size);

        // Reading file...
        char line[256];
        size_t current_offset = 0;

        while (fgets(line, sizeof(line), fp) != NULL) {
            size_t len = strlen(line);

            if (current_offset + len >= f_size) {
                break;
            }

            // Copying lines to shared memory
            memcpy(ptr + current_offset, line, len);
            current_offset += len;
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
            // Executes linux lines read from file

            exec(&ptr);

        } else {
            // continue child process
            
            wait(NULL);

            // manager child stuff            
        }

    } else {
        // -- Parent process --
        // Parent gets processed file from child via piping
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        // Opening new output file to write results into
        FILE *ofp = fopen("output.txt", "w");
        if (ofp == NULL) {
            perror("Output file failed to open");
            exit(1);
        }

        // Writing to output file...
        
        wait(NULL);

        fclose(ofp);
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

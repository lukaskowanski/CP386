// Process Dispatcher
// Libraries
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <fcntl.h>

int dispatch_process(char filename[]) {
    // Parent dispatches children to read and process a given
    // file using shared virtual memory and pipeline techniques

    // Initializing shared memory
    const char *shm_name = "/shared-mem";
    size_t size = 4096;

    // shared mem obj
    int fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open failed to open");
        exit(1);
    }

    // truncates mem size for intial mem objects (broken?)
    // if (ftruncate(fd, size) == -1) {
    //     perror("ftruncate failed");
    //     exit(1);
    // }

    // Mapping shared memory to process address space
    char *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap failed to read");
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
    if (pid == 0) {
        // -- Manager child process --
        // child pipes process to parent
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        // manager child stuff
        // execlp();

        // Child process forks to grandchild
        // pid_t child_pid = fork();

    } else {
        // -- Parent process --
        // Parent gets processed file from child via piping
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        // Parental stuff

        exit(1);
    }

    // Cleaning up and closing shared memory
    munmap(ptr, size);
    close(fd);
    shm_unlink(shm_name);

    return 0;
}

int main() {
    dispatch_process("test.txt");
    return 0;
}
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
#include <string.h>

// Constants
#define SHM_NAME "/shared-mem"
#define MAX_OUT 65536
#define BUF_SIZE (MAX_OUT + 2048)

static int writeOutput(const char *command, const char *output) {
    FILE *fp = fopen("output.txt", "a");
    if (fp == NULL) {
        perror("fopen");
        return -1;
    }
    fprintf(fp, "The output of: %s : is\n", command);
    fprintf(fp, ">>>>>>>>>>>>>>>\n%s<<<<<<<<<<<<<<<\n", output);
    fclose(fp);
    return 0;

}

static void parse_command(char *cmd_str, char *args[]) {
    int i = 0;
    char *token = strtok(cmd_str, " \t\r\n");
    while (token != NULL && i < 63) {
        args[i++] = token;
        token = strtok(NULL, " \t\r\n");
    }
    args[i] = NULL;
}

static void reader_child(const char *filename) {
    // Reader child copies lines from input file into POSIX shared mem
    int in = open(filename, O_RDONLY);
    if (in == -1) {
        perror("file failed to open");
        exit(1);
    }

    struct stat filestat;
    if (fstat(in, &filestat) == -1) {
        perror("fstat failed");
        exit(1);
    }
    size_t size = filestat.st_size;

    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd == -1) {
        perror("shm_open failed");
        exit(1);
    }

    if (ftruncate(fd, size) == -1) {
        perror("ftruncate failed");
        exit(1);
    }

    char *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    size_t offset = 0;
    ssize_t n;
    while (offset < size && (n = read(in, ptr + offset, size - offset)) > 0) {
        offset += n;
    }

    munmap(ptr, size);
    close(fd);
    close(in);
    exit(0);
}

static int exit_code(int status) {
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return 128 + WTERMSIG(status);
}

static void manager_child(char *args1[], char *args2[], int result_fd) {
    int out_pipe[2]; // Final output of job
    int cmd_pipe[2];

    if (pipe(out_pipe) == -1) {
        perror("pipe failed");
        exit(1);
    }
    if (args2 != NULL && pipe(cmd_pipe) == -1) {
        perror("pipe failed");
        exit(1);
    }

    // Command child 1
    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork failed");
        exit(1);
    }
    if (pid1 == 0) {
        if (args2 != NULL) {
            dup2(cmd_pipe[1], STDOUT_FILENO);
        } else {
            dup2(out_pipe[1], STDOUT_FILENO);
        }
        close(out_pipe[0]);
        close(out_pipe[1]);
        if (args2 != NULL) {
            close(cmd_pipe[0]);
            close(cmd_pipe[1]);
        }
        execvp(args1[0], args1);
        _exit(127); // Only reaches if evecvup fails
    }

    // Command child 2
    pid_t pid2 = -1;
    if (args2 != NULL) {
        pid2 = fork();
        if (pid2 < 0) {
            perror("fork failed");
            exit(1);
        }
        if (pid2 == 0) {
            dup2(cmd_pipe[0], STDIN_FILENO);
            dup2(out_pipe[1], STDOUT_FILENO);
            close(out_pipe[0]);
            close(out_pipe[1]);
            close(cmd_pipe[0]);
            close(cmd_pipe[1]);
            execvp(args2[0], args2);
            _exit(127);
        }
        close(cmd_pipe[0]);
        close(cmd_pipe[1]);
    }
    close(out_pipe[1]);

    // Manager child collecting everything for the final output
    static char msg[BUF_SIZE];
    char chunk[4096];
    size_t len = 0;
    ssize_t n;
    while ((n = read(out_pipe[0], chunk, sizeof(chunk))) > 0) {
        if (len < MAX_OUT) {
            size_t room = MAX_OUT - len;
            size_t take = (size_t)n < room ? (size_t)n : room;
            memcpy(msg + len, chunk, take);
            len += take;
        }
    }
    close(out_pipe[0]);

    // Exit statuses
    int status1, status2;
    waitpid(pid1, &status1, 0);
    len += sprintf(msg + len, "[Process status]\nStage 1 (%s): exit %d\n", args1[0], exit_code(status1));
    if (args2 != NULL) {
        waitpid(pid2, &status2, 0);
        len += sprintf(msg + len, "Stage 2 (%s): exit %d\n", args2[0], exit_code(status2));
    }

    // Sending output + status back to parent process
    write(result_fd, msg, len);
    close(result_fd);
    exit(0);
}

int dispatch_process(char filename[]) {
    // Parent dispatches children to read and process a given
    // file using shared virtual memory and pipeline techniques
    // and writes the results into the "output.txt" text file

    // Reader child copies file into shared memory
    pid_t reader_pid = fork();
    if (reader_pid < 0) {
        perror("fork failed");
        exit(1);
    }
    if (reader_pid == 0) {
        reader_child(filename);
    }
    wait(NULL);

    // Parent copies shared mem to a dynamically allocated array
    int fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (fd == -1) {
        perror("shm_open failed");
        exit(1);
    }

    struct stat filestat;
    if (fstat(fd, &filestat) == -1) {
        perror("fstat failed");
        exit(1);
    }
    size_t size = filestat.st_size;

    char *ptr = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    char *data = malloc(size + 1);
    if (data == NULL) {
        perror("malloc failed");
        exit(1);
    }
    memcpy(data, ptr, size);
    data[size] = '\0';

    // Cleaning up shared memory
    munmap(ptr, size);
    close(fd);
    shm_unlink(SHM_NAME);

    FILE *ofp = fopen("output.txt", "w");
    if (ofp == NULL) {
        perror("Output file failed to open");
        exit(1);
    }
    fclose(ofp);

    // Running jobs one by one
    char *line = data;
    while (*line != '\0') {
        char *next = strchr(line, '\n');
        if (next != NULL) {
            *next = '\0';
            next++;
        } else {
            next = line + strlen(line);
        }

        char *cr = strchr(line, '\r');
        if (cr != NULL) {
            *cr = '\0';
        }

        char command[512];
        strncpy(command, line, sizeof(command) - 1);
        command[sizeof(command) - 1] = '\0';

        char *args1[64];
        char *args2[64];
        char *second = strchr(line, '|');
        if (second != NULL) {
            *second = '\0';
            second++;
        }
        parse_command(line, args1);
        if (second != NULL) {
            parse_command(second, args2);
        }

        if (args1[0] == NULL) {
            line = next;
            continue;
        }

        int result_pipe[2];
        if (pipe(result_pipe) == -1) {
            perror("pipe failed");
            exit(1);
        }

        pid_t manager_pid = fork();
        if (manager_pid < 0) {
            perror("fork failed");
            exit(1);
        }
        if (manager_pid == 0) {
            close(result_pipe[0]);
            manager_child(args1, second != NULL ? args2 : NULL, result_pipe[1]);
        }
        close(result_pipe[1]);

        // Parent reads result until manager child is finished
        static char output[BUF_SIZE + 1];
        size_t len = 0;
        ssize_t n;
        while (len < BUF_SIZE - 1 && (n = read(result_pipe[0], output + len, BUF_SIZE - len - 1)) > 0) {
            len += (size_t)n;
        }
        output[len] = '\0';
        close(result_pipe[0]);
        waitpid(manager_pid, NULL, 0);

        writeOutput(command, output);

        line = next;
    }

    free(data);
    return 0;
}

int main(int argc, char *argv[]) {
    char *filename = "sample_in_dispatcher.txt";
    if (argc > 1) {
        filename = argv[1];
    }
    dispatch_process(filename);

    return 0;
}

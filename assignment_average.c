// Assignment Average
// Libraries
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// Global variables
#define MHEIGHT 10
#define MWIDTH 2

int calculate_avg(char *assignment_name) {
    // Calculates the average grade for each assignment for all chapters using
    // Teacher, GTA and TA processes

    FILE *fp = fopen(assignment_name, "r");
    if (fp == NULL) {
        perror("file failed to open");
        exit(1);
    }

    // Teacher inits gta via fork
    pid_t tpid = fork();
    if (tpid < 0) {
        perror("fork failed");
        exit(1);
    }

    if (tpid == 0) {
        // Gta forks to ta
        pid_t tapid = fork();
        if (tapid < 0) {
            perror("fork failed");
            exit(1);
        }

        else if (tapid == 0) {
            float matrix[MHEIGHT][MWIDTH];

            // int num1, num2, avg, total, count;
            int width, height, i, j;

            // Ta process (grandchild)
            for (j = 0; j < height; j++) {
                for (i = 0; i < width; i++) {
                    printf("%10.1f", &matrix[j][i]);
                }
                printf("\n");
            }
            

        } else {
            // continue gta (child) process
            wait(NULL);

            printf("This is child process\n");

        }

    } else {
        // Teacher process (parent)
        wait(NULL);

        printf("Assignment 1 - Average = 10\n");
        printf("Assignment 2 - Average = 12\n");

    }

    fclose(fp);

    return 0;
}

int main() {
    calculate_avg("sample_in_grades.txt");

    return 0;
}

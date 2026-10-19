// Assignment Average
// Libraries
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// Global variables
#define MAXROWS 100
#define MAXCOLS 10

int calculate_avg(char *assignment_name) {
    // Calculates the average grade for each assignment for all chapters using
    // Teacher, GTA and TA processes

    FILE *fp = fopen(assignment_name, "r");
    if (fp == NULL) {
        perror("file failed to open");
        exit(1);
    }

    // Parent processses file into 2D matrix
    int matrix[MAXROWS][MAXCOLS];
    int num_rows = 0;
    int num_cols = 0;
    char line[1024];

    while (fgets(line, sizeof(line), fp) != NULL) {
        int offset = 0, scanned = 0, val;
        int col = 0;
        
        while (sscanf(line + offset, "%d%n", &val, &scanned) == 1) {
            matrix[num_rows][col] = val;
            offset += scanned;
            col++;
        }
        if (col > 0) {
            if (num_cols == 0) {
                num_cols = col;
            }
            num_rows++;
        }
    }
    fclose(fp);

    // 2 assignments per chapter
    int num_chapters = num_cols / 2;

    // Teacher inits gtas via fork (dynamically)
    for (int i = 0; i < num_chapters; i++) {
        int pipe_t2g[2];
        if (pipe(pipe_t2g) == -1) {
            perror("pipe failed");
            exit(1);
        }

            pid_t gpid = fork();
    if (gpid < 0) {
        perror("fork failed");
        exit(1);
    }

    if (gpid == 0) {
        // Gradta process (child)
        close(pipe_t2g[1]); // Closing write

        // Reading chapter data
        int chapter_data[MAXROWS][2];
        for (int i = 0; i < num_rows; i++) {
            read(pipe_t2g[0], &chapter_data[i][0], sizeof(int));
            read(pipe_t2g[0], &chapter_data[i][1], sizeof(int));
        }
        close(pipe_t2g[0]);

        // Each gta creates 2 ta processes
        for (int j = 0; j < 2; j++) {
            int pipe_g2ta[2];
            if (pipe(pipe_g2ta) == -1) {
                perror("pipe filed");
                exit(1);
            }

            // Gta forks to ta
            pid_t tapid = fork();
            if (tapid < 0) {
                perror("fork failed");
                exit(1);
            }

            if (tapid == 0) {
                // Ta process (grandchild)
                close(pipe_g2ta[1]); // Close write

                int assignment_grades[MAXROWS];
                for (int i = 0; i < num_rows; i++) {
                    read(pipe_g2ta[0], &assignment_grades[i], sizeof(int));
                }
                close(pipe_g2ta[0]);

                // Calculating average per assignment
                float sum = 0;
                for (int a = 0; a < num_rows; a++) {
                    sum += assignment_grades[a];
                }
                float avg = sum / num_rows;

                // Calculating overall assignment number
                int assignment_num = i * 2 + j + 1;
                printf("Assignment %d - Average = %f\n", assignment_num, avg);

                exit(0);

            } else {
                // continue gta (child) process
                close(pipe_g2ta[0]);

                // Send assignment column data to TA
                for (int r = 0; r < num_rows; r++) {
                    write(pipe_g2ta[1], &chapter_data[r][j], sizeof(int));
                }
                close(pipe_g2ta[1]);

                wait(NULL);                
            }
        }
        exit(0);
    } else {
        // Teacher process (parent)
        close(pipe_t2g[0]); // Close reading

        // Sending 2 cols per chapter to gta
        for (int r = 0; r < num_rows; r++) {
            write(pipe_t2g[1], &matrix[r][i * 2], sizeof(int));
            write(pipe_t2g[1], &matrix[r][ i * 2 + 1], sizeof(int));
        }
        close(pipe_t2g[1]);
        }
    }

    // Teacher waits for all gtas to finish
    for (int i = 0; i < num_chapters; i++) {
        wait(NULL);
    }

    return 0;
}

int main() {
    calculate_avg("sample_in_grades.txt");

    return 0;
}

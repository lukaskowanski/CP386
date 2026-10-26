// Assignment Average
// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// Global variables
#define MAXROWS 100
#define MAXCOLS 20

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

    while (fgets(line, sizeof(line), fp) != NULL && num_rows < MAXROWS) {
        int offset = 0, scanned = 0, val;
        int col = 0;
        
        while (sscanf(line + offset, "%d%n", &val, &scanned) == 1 && col < MAXCOLS) {
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

    // 2 assignments per chapter, optional 1 assignment for the last chapter
    int num_chapters = (num_cols + 1) / 2;

    // Teacher inits GTAs via fork (dynamically)
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
        // GTA process (child)
        close(pipe_t2g[1]); // Closing write

        // accommodate odd number of assignments/columns
        // by reading chapter data (2 columns, optioanl 1 column for the last chapter)
        int chapter_data[MAXROWS][2];
        for (int r = 0; r < num_rows; r++) {
            for (int j = 0; j < 2 && i * 2 + j < num_cols; j++) {
                read(pipe_t2g[0], &chapter_data[r][j], sizeof(int));
            }
        }
        close(pipe_t2g[0]);

        // Each GTA creates 2 TA processes (optional 1 TA process for the last chapter)
        for (int j = 0; j < 2 && i * 2 + j < num_cols; j++) {
            int pipe_g2ta[2];
            if (pipe(pipe_g2ta) == -1) {
                perror("pipe filed");
                exit(1);
            }

            // GTA forks to TA
            pid_t tapid = fork();
            if (tapid < 0) {
                perror("fork failed");
                exit(1);
            }

            if (tapid == 0) {
                // TA process (grandchild)
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
                // Continue GTA (child) process
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

        // Sending 2 columns per chapter to GTA (optional 1 for the last chapter)
        for (int r = 0; r < num_rows; r++) {
            for (int j = 0; j < 2 && i * 2 + j < num_cols; j++) {
                write(pipe_t2g[1], &matrix[r][i * 2 + j], sizeof(int));
            }
        }
        close(pipe_t2g[1]);
        wait(NULL);
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    // Fix autograder error: missing input filename prints usage (no filename given)
    if (argc != 2) {
        // Lukas
        // printf("Usage: %s <grades file>\n", argv[0]);
        printf("Usage: ./assignment_average [sample_in_grades.txt]\n");
        return 1;
    }

    // Fix autograder error: filename given but cannot open (missing file, wrong name, no permission)
    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("Failed to access %s\n", argv[1]);
        return 0;
    }
    fclose(fp);

    // Otherwise call calculate_avg
    calculate_avg(argv[1]);

    return 0;
}

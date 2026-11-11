// Assignment 3 - Question 1 - Sudoku Validator

// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define INPUT_FILE "sample_in_sudoku.txt"
#define SIZE 9
#define NUM_THREADS 27

int sudoku[SIZE][SIZE]; // sudoku grid dimensions
int valid[NUM_THREADS]; // number of threads to run validation check

// set up struct to pass parameters
typedef struct
{
    int row;
    int column;
} parameters;

// worker thread that checks row validity
void *check_row(void *param)
{
    parameters *data = (parameters *) param;
    int row = data->row;
    free(data);

    int seen[SIZE + 1] = {0};
    for (int col = 0; col < SIZE; col++) {
        int num = sudoku[row][col];
        if (num < 1 || num > SIZE || seen[num]) {
            valid[row] = 0;
            pthread_exit(NULL);
        }
        seen[num] = 1;
    }
    valid[row] = 1;
    pthread_exit(NULL);
}

// worker thread that checks column validity
void *check_column(void *param)
{
    parameters *data = (parameters *) param;
    int col = data->column;
    free(data);

    int seen[SIZE + 1] = {0};
    for (int row = 0; row < SIZE; row++) {
        int num = sudoku[row][col];
        if (num < 1 || num > SIZE || seen[num]) {
            valid[SIZE + col] = 0;
            pthread_exit(NULL);
        }
        seen[num] = 1;
    }
    valid[SIZE + col] = 1;
    pthread_exit(NULL);
}

// worker thread that checks sub-grid validtiy
void *check_subgrid(void *param)
{
    parameters *data = (parameters *) param;
    int start_row = data->row;
    int start_col = data->column;
    free(data);

    int index = 2 * SIZE + (start_row / 3) * 3 + (start_col / 3);
    int seen[SIZE + 1] = {0};

    for (int row = start_row; row < start_row + 3; row++) {
        for (int col = start_col; col < start_col + 3; col++) {
            int num = sudoku[row][col];
            if (num < 1 || num > SIZE || seen[num]) {
                valid[index] = 0;
                pthread_exit(NULL);
            }
            seen[num] = 1;
        }
    }
    valid[index] = 1;
    pthread_exit(NULL);
}

// helper function that checks if input file is empty
static int file_is_empty(FILE *fp)
{
    int ch;
    while ((ch = fgetc(fp)) != EOF) {
        if (!isspace(ch)) {
            rewind(fp);
            return 0;
        }
    }
    rewind(fp);
    return 1;
}

// helper function that reads input file
static int read_sudoku(const char *filename)
{
    FILE *fp = fopen(filename, "r");

    // raise error when input file does not exist, wrong name, no access
    if (fp == NULL) {
        fprintf(stderr, "Error: Unable to open input file \"%s\": %s\n",
                filename, strerror(errno));
        return -1;
    }

    // raise error when input file is empty
    if (file_is_empty(fp)) {
        fprintf(stderr, "Error: Input file is empty\n");
        fclose(fp);
        return -1;
    }

    // read input file and further checks for sudoku grid component errors
    for (int row = 0; row < SIZE; row++) {
        for (int col = 0; col < SIZE; col++) {

            // raise error when sudoku grid has dimensions smaller than 9x9
            int rc = fscanf(fp, "%d", &sudoku[row][col]);
            if (rc == EOF) {
                fprintf(stderr, "Error: Given Sudoku grid is incomplete (ended at row %d, column %d)\n",
                        row + 1, col + 1);
                fclose(fp);
                return -1;
            }
            // raise error when a component is non-numeric
            if (rc != 1) {
                fprintf(stderr, "Error: Given Sudoku grid has non-numeric component at row %d, column %d\n",
                    row + 1, col + 1);
                fclose(fp);
                return -1;
            }
        }
    }

    // raise error when sudoku grid has dimension bigger than 9x9
    int extra;
    if (fscanf(fp, "%d", &extra) == 1)
        fprintf(stderr, "Warning: Given Sudoku grid has dimension bigger than 9x9\n"
                        "Extra data ignored\n");

    fclose(fp);
    return 0;
}

// helper function to print sudoku grid
static void print_sudoku(void)
{
    for (int row = 0; row < SIZE; row++) {
        for (int col = 0; col < SIZE; col++)
            printf("%d%s", sudoku[row][col], col < SIZE - 1 ? "   " : "\n");
    }
}

// parent thread
static int start_worker(pthread_t *tid, void *(*worker)(void *), int row, int column)
{
    parameters *data = (parameters *) malloc(sizeof(parameters));
    if (data == NULL) {
        fprintf(stderr, "Error: out of memory allocating thread parameters\n");
        return -1;
    }
    data->row = row;
    data->column = column;

    int rc = pthread_create(tid, NULL, worker, data);
    if (rc != 0) {
        fprintf(stderr, "Error: Unable to create thread: %s\n", strerror(rc));
        free(data);
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    const char *filename = (argc > 1) ? argv[1] : INPUT_FILE;

    if (read_sudoku(filename) != 0)
        return EXIT_FAILURE;

    pthread_t workers[NUM_THREADS];
    int created = 0;
    int failed = 0;

    // run worker threads that check rows
    for (int i = 0; i < SIZE && !failed; i++) {
        if (start_worker(&workers[created], check_row, i, 0) == 0)
            created++;
        else
            failed = 1;
    }

    // run worker threads that check columns
    for (int i = 0; i < SIZE && !failed; i++) {
        if (start_worker(&workers[created], check_column, 0, i) == 0)
            created++;
        else
            failed = 1;
    }

    // run worker threads that check subgrids
    for (int i = 0; i < SIZE && !failed; i += 3) {
        for (int j = 0; j < SIZE && !failed; j += 3) {
            if (start_worker(&workers[created], check_subgrid, i, j) == 0)
                created++;
            else
                failed = 1;
        }
    }

    // join worker threads after done running
    for (int i = 0; i < created; i++) {
        int rc = pthread_join(workers[i], NULL);
        if (rc != 0) {
            fprintf(stderr, "Error: Unable to join worker threads %d: %s\n",
                    i, strerror(rc));
            failed = 1;
        }
    }
    if (failed) {
        fprintf(stderr, "Validation process was not completed\n");
        return EXIT_FAILURE;
    }

    // check validity in the final binary result array
    // valid if all array components are 1
    int is_valid = 1;
    for (int i = 0; i < NUM_THREADS; i++) {
        if (valid[i] != 1) {
            is_valid = 0;
            break;
        }
    }

    
    printf("Sudoku Puzzle Solution is:\n");
    print_sudoku();
    printf("Sudoku puzzle is %s\n", is_valid ? "valid" : "invalid");

    return EXIT_SUCCESS;
}
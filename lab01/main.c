#include "mpi.h"
#include <stdio.h>
#include <string.h>

#include "path_search.h"

int pid = MPI_ANY_SOURCE;

int program_init(int argc, char *argv[], int *num_proc, int *proc_id) {
    int rc;

    if ((rc = MPI_Init(&argc, &argv)) != MPI_SUCCESS) {
        printf("Init error, exiting ...");
        MPI_Abort(MPI_COMM_WORLD, rc);
    } else if ((rc = MPI_Comm_size(MPI_COMM_WORLD, num_proc)) != MPI_SUCCESS) {
        printf("Get comm size, exiting ...");
        MPI_Abort(MPI_COMM_WORLD, rc);
    } else if ((rc = MPI_Comm_rank(MPI_COMM_WORLD, proc_id)) != MPI_SUCCESS) {
        printf("Get comm rank, exiting ...");
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    return rc;
}

int main(int argc, char *argv[]) {
    size_t board_height;
    size_t board_width;
    int rc = OK;
    int num_proc = 0;
    char *end = NULL;

    switch (argc) {
        case 3:
            if ((board_height = strtol(argv[1], &end, 10)) < 1 || *end != '\0') {
                printf("\ninvalid board height value\n");
                return ERR_INVALID_ARGV;
            }
            if ((board_width = strtol(argv[2], &end, 10)) < 1 || *end != '\0') {
                printf("\ninvalid board width value\n");
                return ERR_INVALID_ARGV;
            }
            break;
        case 2:
            if ((board_height = board_width = strtol(argv[1], &end, 10)) < 1 || *end != '\0') {
                printf("\ninvalid board size value\n");
                return ERR_INVALID_ARGV;
            }
            break;
        default:
            board_height = board_width = 20;
    }

    board_t board = create();

    if (!(rc = board.ops.init(&board, default_board_ops(), board_height,
                              board_width))) {                      // memory init
        if ((rc = program_init(argc, argv, &num_proc, &pid)) == MPI_SUCCESS) {                  // MPI init
            double start_time, end_time;

            if (pid == LEAD_PROC_ID) {
                start_time = MPI_Wtime();
            }

            rc = find_path_parallel_greedy(&board, num_proc);

            if (pid == LEAD_PROC_ID) {
                end_time = MPI_Wtime();
            }

            if (!rc && pid == LEAD_PROC_ID) {   // search
                if (path_is_correct(&board)) {                                                  // path check
                    printf("\nPATH FOUND\n");
                    printf("\nTIME SPENT: %lf sec\n", end_time - start_time);

                    if (board_height <= 30 && board_width <= 30) {
                        board.ops.print(&board);
                    }

                    if (!(rc = board.ops.fprint(&board, "out"))) {
                        printf("\nRESULT SAVED IN FILE\n");
                    } else {
                        printf("\nERROR WHILE SAVING RESULT TO FILE\n");
                    }
                } else {
                    printf("\nPATH NOT FOUND\n");
                }
            }

            MPI_Abort(MPI_COMM_WORLD, OK);
        }

        board.ops.dispose(&board);
    }

    return rc;
}
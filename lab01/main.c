#include "mpi.h"
#include <stdio.h>
#include <string.h>

#include "path_search.h"

int pid = MPI_ANY_SOURCE;
int num_proc = -1;

err_code_t program_init(int argc, char *argv[]) {
    int rc;

    if ((rc = MPI_Init(&argc, &argv)) != MPI_SUCCESS) {
        printf("Init error, exiting ...");
        MPI_Abort(MPI_COMM_WORLD, rc);
    } else if ((rc = MPI_Comm_size(MPI_COMM_WORLD, &num_proc)) != MPI_SUCCESS) {
        printf("Get comm size, exiting ...");
        MPI_Abort(MPI_COMM_WORLD, rc);
    } else if ((rc = MPI_Comm_rank(MPI_COMM_WORLD, &pid)) != MPI_SUCCESS) {
        printf("Get comm rank, exiting ...");
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    return rc;
}

err_code_t main(int argc, char *argv[]) {
    size_t board_height;
    size_t board_width;
    int rc = OK;
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

    if ((rc = board.ops.init(&board, default_board_ops(), board_height,
                             board_width)) == OK) {
        if ((rc = program_init(argc, argv)) == MPI_SUCCESS) {
            double start_time, end_time;
            pos_t prefix[100];

            prefix[0].x = 0;
            prefix[0].y = 0;

            start_time = MPI_Wtime();
            rc = find_path_parallel(&board,
                                    prefix,
                                    num_proc > POSSIBLE_MOVES_NUM
                                        ? num_proc
                                        : POSSIBLE_MOVES_NUM,
                                    0);

            //MPI_Barrier(MPI_COMM_WORLD);

            if (rc == PATH_FOUND) {
                end_time = MPI_Wtime();
                printf("\nPID=%d: PATH FOUND, VALIDATING\n", pid);
                if (path_is_correct(&board)) {
                    printf("\nPID=%d: PATH IS VALID\n", pid);
                    printf("\nTIME SPENT: %lf SEC\n", end_time - start_time);

                    if (board_height <= 30 && board_width <= 30) {
                        board.ops.print(&board);
                    }

                    char filename[100];

                    sprintf(filename, "out#%d", pid);

                    if (!(rc = board.ops.fprint(&board, filename))) {
                        printf("\nRESULT SAVED IN FILE out\n");
                    } else {
                        printf("\nERROR WHILE SAVING RESULT TO FILE\n");
                    }
                } else {
                    printf("\nPATH FOUND BUT NOT VALID\n"); // must be unreachable
                }
            } else if (rc == PATH_FOUND_ANOTHER) {
                printf("\npid = %d: PATH FOUND BY ANOTHER PROCESS\n", pid);
            } else if (rc == PATH_NOT_FOUND) {
                printf("\npid = %d: PATH NOT FOUND\n", pid);
            } else {
                error_code_handler(rc);
            }

            MPI_Barrier(MPI_COMM_WORLD);

            MPI_Finalize();
        }

        board.ops.dispose(&board);
    }

    return rc;
}
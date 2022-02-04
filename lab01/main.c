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

    char proc_name[MPI_MAX_PROCESSOR_NAME];
    int len;

    MPI_Get_processor_name(proc_name, &len);

    printf("\nPID=%d: PROC_NAME=%s\n", pid, proc_name);

    return rc;
}

err_code_t main(int argc, char *argv[]) {
    size_t board_height = 20;
    size_t board_width = 20;
    size_t init_x = 0, init_y = 0;
    int rc = OK;
    char *end = NULL;

    switch (argc) {
        case 5:
            if ((board_height = strtol(argv[3], &end, 10)) < 1 || *end != '\0') {
                printf("\ninvalid board height value\n");
                return ERR_INVALID_ARGV;
            }
            if ((board_width = strtol(argv[4], &end, 10)) < 1 || *end != '\0') {
                printf("\ninvalid board width value\n");
                return ERR_INVALID_ARGV;
            }
            if ((init_x = strtol(argv[1], &end, 10)) < 0 || init_x >= board_width || *end != '\0') {
                printf("\ninvalid init x value\n");
                return ERR_INVALID_ARGV;
            }
            if ((init_y = strtol(argv[2], &end, 10)) < 0 || init_y >= board_height || *end != '\0') {
                printf("\ninvalid init y value\n");
                return ERR_INVALID_ARGV;
            }
            break;
        case 4:
            if ((board_height = board_width = strtol(argv[3], &end, 10)) < 1 || *end != '\0') {
                printf("\ninvalid board size value\n");
                return ERR_INVALID_ARGV;
            }
            if ((init_x = strtol(argv[1], &end, 10)) < 0 || init_x >= board_width || *end != '\0') {
                printf("\ninvalid init x value\n");
                return ERR_INVALID_ARGV;
            }
            if ((init_y = strtol(argv[2], &end, 10)) < 0 || init_y >= board_height || *end != '\0') {
                printf("\ninvalid init y value\n");
                return ERR_INVALID_ARGV;
            }
            break;
        case 3:
            if ((init_x = strtol(argv[1], &end, 10)) < 0 || init_x >= board_width || *end != '\0') {
                printf("\ninvalid init x value\n");
                return ERR_INVALID_ARGV;
            }
            if ((init_y = strtol(argv[2], &end, 10)) < 0 || init_y >= board_height || *end != '\0') {
                printf("\ninvalid init y value\n");
                return ERR_INVALID_ARGV;
            }
            break;
        case 2:
            printf("\ninvalid arg num\n");
            return ERR_INVALID_ARGV;
        default:
            break;
    }

    board_t board = create();

    if ((rc = board.ops.init(&board, default_board_ops(), board_height,
                             board_width)) == OK) {
        if ((rc = program_init(argc, argv)) == MPI_SUCCESS) {
            double start_time, end_time;
            pos_t prefix[100];

            prefix[0].x = init_x;
            prefix[0].y = init_y;

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
                    printf("\nPID=%d: TIME SPENT: %lf SEC\n", pid, end_time - start_time);

                    /*if (board_height <= 30 && board_width <= 30) {
                        board.ops.print(&board);
                    }*/

                    char filename[100];

                    sprintf(filename, "out#%d", pid);

                    if (!(rc = board.ops.fprint(&board, filename))) {
                        printf("\nPID=%d: RESULT SAVED IN FILE out\n", pid);
                    } else {
                        printf("\nPID=%d: ERROR WHILE SAVING RESULT TO FILE\n", pid);
                    }
                } else {
                    printf("\nPID=%d: PATH FOUND BUT NOT VALID\n", pid); // must be unreachable
                }
            } else if (rc == PATH_FOUND_ANOTHER) {
                printf("\nPID = %d: PATH FOUND BY ANOTHER PROCESS\n", pid);
            } else if (rc == PATH_NOT_FOUND) {
                printf("\nPID = %d: PATH NOT FOUND\n", pid);
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
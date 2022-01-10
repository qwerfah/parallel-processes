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
    size_t board_height = 20;
    size_t board_width = 10;
    int rc = OK;
    int num_proc = 0;

    board_t board = create();

    if (!(rc = board.ops.init(&board, default_board_ops(), board_height, board_width))) {                      // memory init
        if ((rc = program_init(argc, argv, &num_proc, &pid)) == MPI_SUCCESS) {                  // MPI init
            if (!(rc = find_path_parallel_greedy(&board, num_proc)) && pid == LEAD_PROC_ID) {   // search
                if (path_is_correct(&board)) {                                                  // path check
                    printf("\nPATH FOUND\n");
                    board.ops.print(&board);

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
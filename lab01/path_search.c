//
// Created by Karpukhin Aleksandr on 03.01.2022.
//

#include "path_search.h"
#include "string.h"
#include "task.h"

#include <time.h>

static MPI_Request recv_request = MPI_ANY_TAG;
static MPI_Request send_request = MPI_ANY_TAG;
static int recv_code = PATH_NOT_FOUND;
static int send_code = PATH_FOUND;

const offset_t possible_moves[POSSIBLE_MOVES_NUM] = {{2,  1},
                                                     {2,  -1},
                                                     {1,  -2},
                                                     {-1, -2},
                                                     {-2, -1},
                                                     {-2, 1},
                                                     {-1, 2},
                                                     {1,  2}};

_Bool move_possible(board_t *board, pos_t pos, offset_t offset, pos_t *new_pos) {
    long long x = ((long long) pos.x) + (long long) offset.x_offset;
    long long y = ((long long) pos.y) + (long long) offset.y_offset;

    if (x >= 0 && y >= 0 && x < board->width && y < board->height) {
        new_pos->x = x;
        new_pos->y = y;
        return !board->ops.get(board, *new_pos);
    }

    return FALSE;
}

pos_t move(pos_t pos, offset_t offset) {
    pos_t new_pos = {((int) pos.x) + offset.x_offset, ((int) pos.y) + offset.y_offset};
    return new_pos;
}

int get_moves_count(board_t *board, pos_t pos) {
    int moves_count = 0;
    pos_t temp;

    for (int i = 0; i < POSSIBLE_MOVES_NUM; i++) {
        moves_count += move_possible(board, pos, possible_moves[i], &temp);
    }

    return moves_count;
}

_Bool try_find_next_pos(board_t *board, pos_t curr_pos, pos_t *next_pos) {
    int moves_count = 0;
    int offset_i = -1;
    int i = 0;

    // Find first possible not used move
    for (; i < POSSIBLE_MOVES_NUM; i++) {
        if (move_possible(board, curr_pos, possible_moves[i], next_pos)) {
            moves_count = get_moves_count(board, *next_pos);
            offset_i = i;
            break;
        }
    }

    // Search for best next not used move
    for (i += 1; i < POSSIBLE_MOVES_NUM; i++) {
        pos_t new_pos;
        if (move_possible(board, curr_pos, possible_moves[i], &new_pos)) {
            int new_pos_moves_count = get_moves_count(board, new_pos);
            if (new_pos_moves_count < moves_count) {
                moves_count = new_pos_moves_count;
                *next_pos = new_pos;
                offset_i = i;
            }
        } else {

        }
    }

    return (offset_i >= 0);
}

err_code_t check(void) {
    int rc = OK;
    int flag = FALSE;
    MPI_Status status;

    if ((rc = MPI_Iprobe(MPI_ANY_SOURCE, pid, MPI_COMM_WORLD, &flag, &status)) < 0) {
        MPI_Abort(MPI_COMM_WORLD, rc);
        return rc;
    }

    if (flag == TRUE) {
        return ((recv_code == PATH_FOUND) ? PATH_FOUND_ANOTHER : ERR_UNKNOWN_CODE);
    }

    return PATH_NOT_FOUND;
}

err_code_t find_path(board_t *board, pos_t pos, unsigned move_num) {
    int rc = OK;

    pos_t prev_pos;

    pos_t *stack = calloc(board->max_moves, sizeof(pos_t));
    int sp = 0;

    while (1) {
        if ((rc = check()) != PATH_NOT_FOUND) {
            free(stack);
            return rc;
        }

        board->ops.set(board, pos, move_num);
        if (move_num == board->max_moves) {
            send_code = PATH_FOUND;

            for (int i = 0; i < num_proc; i++) {
                if (i != pid && (rc = MPI_Send(&send_code,
                                               1,
                                               MPI_INT,
                                               i,
                                               i,
                                               MPI_COMM_WORLD)) < 0) {
                    MPI_Abort(MPI_COMM_WORLD, rc);
                    free(stack);
                    return rc;
                }
            }

            free(stack);
            return PATH_FOUND;
        }

        pos_t next_pos;

        if (try_find_next_pos(board, pos, &next_pos)) {
            if ((rc = check()) != PATH_NOT_FOUND) {
                free(stack);
                return rc;
            }

            stack[sp++] = pos;
            pos = next_pos;
            move_num++;
            continue;
        }

        board->ops.set(board, pos, NOT_IN_PATH);
        pos = stack[sp--];
        move_num--;
    }
}

// Parallel Euler search: different branches in search tree is processed by different processes
// Less effective because of strong dependency of initial search position
err_code_t find_path_parallel(board_t *board, pos_t *pos, unsigned task_num, unsigned move_idx) {
    if (pos == NULL) {
        return ERR_NULL_POINTER;
    } else if (task_num == 0) {
        return ERR_INVALID_TASK_NUM;
    }

    int rc = OK;
    static int curr_pid = 0;

    if (pid == LEAD_PROC_ID) {
        printf("\nLEAD: SPLIT TASKS FOR %d PROCESSES STAGE=%d\n", num_proc, move_idx);
        board->ops.set(board, pos[move_idx], move_idx + 1);

        task_t task = {.pid = MPI_ANY_SOURCE, .prefix = {0}, .prefix_length = 0};

        // Create tasks for parallel processes
        while (task_num > 0) {
            if (try_find_next_pos(board, pos[move_idx], &(pos[move_idx + 1]))) {
                if (task.pid != MPI_ANY_SOURCE) {
                    if ((rc = add_task_to_list(&task)) != OK) {
                        return rc;
                    }
                    task_num--;
                    print_task(&task);
                    printf("\nLEAD: TASK FOR PID=%d CREATED, %d TASKS LEFT\n", curr_pid, task_num);
                    curr_pid = (curr_pid + 1) % num_proc;
                }

                board->ops.set(board, pos[move_idx + 1], move_idx + 1);
                task = task_ctr(curr_pid, pos, move_idx + 2);
            } else {
                return find_path_parallel(board, pos, task_num, move_idx + 1);
            }
        }

        // Send tasks to all processes except lead
        if ((rc = schedule_tasks()) != OK) {
            return rc;
        }
    } else {
        if ((rc = accept_tasks()) != OK) {
            return rc;
        }
    }

    if ((rc = MPI_Irecv(&recv_code,
                        1,
                        MPI_INT,
                        MPI_ANY_SOURCE,
                        pid,
                        MPI_COMM_WORLD,
                        &recv_request)) < 0) {
        MPI_Abort(MPI_COMM_WORLD, rc);
        return rc;
    }

    return execute_tasks(board, find_path);
}

// Check if move is possible in solved matrix
// (no new position returns and no matrix cell value checks)
_Bool test_move_possible(board_t *board, pos_t pos, offset_t offset) {
    long long x = ((long long) pos.x) + (long long) offset.x_offset;
    long long y = ((long long) pos.y) + (long long) offset.y_offset;
    return (x >= 0 && y >= 0 && x < board->width && y < board->height);
}

// Check if the solved matrix is correct
_Bool path_is_correct(board_t *board) {
    int i, j;
    int value = 0;
    pos_t pos;

    // Find initial position (must be 1 in corresponding matrix cell)
    for (i = 0; i < board->height && value != 1; i++) {
        for (j = 0; j < board->width && value != 1; j++) {
            pos.x = j;
            pos.y = i;
            value = board->ops.get(board, pos);
        }
    }

    if (value != 1) {
        printf("\nINVALID VALUE IN INIT POS: INIT = %d\n", value);
        return FALSE;
    }

    for (i = 2; i < board->max_moves; i++) {
        _Bool next_pos_found = FALSE;
        pos_t next_pos;

        for (j = 0; j < POSSIBLE_MOVES_NUM && !next_pos_found; j++) {
            if (test_move_possible(board, pos, possible_moves[j])) {
                next_pos = move(pos, possible_moves[j]);
                value = board->ops.get(board, next_pos);
                if (value == i) {
                    next_pos_found = TRUE;
                } else if (value == 0) {
                    board->ops.set(board, next_pos, i);
                    next_pos_found = TRUE;
                }
            }
        }

        if (next_pos_found) {
            pos = next_pos;
        } else {
            for (j = 0; j < POSSIBLE_MOVES_NUM && !next_pos_found; j++) {
                if (test_move_possible(board, pos, possible_moves[j])) {
                    next_pos = move(pos, possible_moves[j]);
                    printf("\nINVALID VALUE IN NEXT POS: PREV = %d NEXT = %d\n",
                           board->ops.get(board, pos),
                           board->ops.get(board, next_pos));
                }
            }

            return FALSE;
        }
    }

    return TRUE;
}


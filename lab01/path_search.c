//
// Created by Karpukhin Aleksandr on 03.01.2022.
//

#include "path_search.h"
#include "string.h"
#include <time.h>

MPI_Request request = MPI_ANY_TAG;

const offset_t possible_moves[POSSIBLE_MOVES_NUM] = {{2,  1},
                                                     {2,  -1},
                                                     {1,  -2},
                                                     {-1, -2},
                                                     {-2, -1},
                                                     {-2, 1},
                                                     {-1, 2},
                                                     {1,  2}};

_Bool move_possible(board_t *board, pos_t pos, offset_t offset, pos_t *new_pos) {
    int x = ((int) pos.x) + offset.x_offset;
    int y = ((int) pos.y) + offset.y_offset;

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

err_code_t find_path(board_t *board, pos_t pos, unsigned move_num) {
    int rc = OK;

    if (pid == LEAD_PROC_ID) {
        int flag;
        MPI_Status status;

        if ((rc = MPI_Test(&request, &flag, &status)) < 0) {
            MPI_Abort(MPI_COMM_WORLD, rc);
            return rc;
        }

        if (flag) {
            // printf("lead proc: path found\n");
            return PATH_FOUND;
        }
    }

    board->ops.set(board, pos, move_num);
    if (move_num == board->max_moves) {
        if (pid != LEAD_PROC_ID && (rc = MPI_Send(board->raw,
                                                  board->max_moves,
                                                  MPI_UNSIGNED,
                                                  LEAD_PROC_ID,
                                                  0,
                                                  MPI_COMM_WORLD)) < 0) {
            MPI_Abort(MPI_COMM_WORLD, rc);
            return rc;
        }

        // printf("child proc with pid = %d: path found\n", pid);
        return PATH_FOUND;
    }

    pos_t next_pos;

    while (try_find_next_pos(board, pos, &next_pos)) {
        if (find_path(board, next_pos, move_num + 1) == PATH_FOUND) {
            if (pid != LEAD_PROC_ID && (rc = MPI_Send(board->raw,
                                                      board->max_moves,
                                                      MPI_UNSIGNED,
                                                      LEAD_PROC_ID,
                                                      0,
                                                      MPI_COMM_WORLD)) < 0) {
                MPI_Abort(MPI_COMM_WORLD, rc);
                return rc;
            }

            // printf("child proc with pid = %d: path found\n", pid);
            return PATH_FOUND;
        }
    }

    board->ops.set(board, pos, NOT_IN_PATH);
    return PATH_NOT_FOUND;
}

int find_path_continuation(board_t *board, pos_t *path, size_t path_len) {
    int i = 0;
    int rc = OK;

    if (pid == LEAD_PROC_ID) {
        // board_t new_board;
        // init(&new_board, default_board_ops(), board->size);
        // MPI_Request request;

        if ((rc = MPI_Irecv(board->raw,
                            board->max_moves,
                            MPI_UNSIGNED,
                            MPI_ANY_SOURCE,
                            0,
                            MPI_COMM_WORLD,
                            &request)) < 0) {
            MPI_Abort(MPI_COMM_WORLD, rc);
            return rc;
        }
    }

    for (; i < path_len - 1; i++) {
        board->ops.set(board, path[i], i + 1);
    }

    return find_path(board, path[i], i + 1);
}

_Bool pos_in_array(pos_t pos, pos_t *pos_array, int pos_num) {
    if (!pos_array) return FALSE;

    for (int i = 0; i < pos_num; i++) {
        if (pos_array[i].x == pos.x && pos_array[i].y == pos.y) return TRUE;
    }

    return FALSE;
}

pos_t generate_pos(pos_t *pos_array, int pos_num, size_t height, size_t width) {
    pos_t pos;

    do {
        pos.x = rand() % width;
        pos.y = rand() % height;
    } while (pos_in_array(pos, pos_array, pos_num));

    pos_array[pos_num] = pos;

    return pos;
}


err_code_t find_path_parallel_greedy(board_t *board, int num_proc, pos_t *init_pos) {
    int rc = OK;
    pos_t pos_array[num_proc];

    if (pid == LEAD_PROC_ID) {
        printf("lead proc with pid = %d split task stage for %d processes\n", pid, num_proc);
        srand(time(0));
        for (int child_pid = 1; child_pid < num_proc; child_pid++) {
            pos_t pos = generate_pos(pos_array, child_pid - 1, board->height, board->width);

            printf("lead proc with pid = %d; init pos generated: (%d, %d)\n", pid, pos.x, pos.y);
            if ((rc = MPI_Send(&pos, sizeof(pos_t), MPI_BYTE, child_pid, 0, MPI_COMM_WORLD)) < 0) {
                MPI_Abort(MPI_COMM_WORLD, rc);
                return rc;
            }
        }

        *init_pos = generate_pos(pos_array, num_proc - 1, board->height, board->width);
        printf("lead proc with pid = %d; init pos generated: (%d, %d)\n", pid, init_pos->x, init_pos->y);

        if ((rc = MPI_Irecv(board->raw,
                            board->max_moves,
                            MPI_UNSIGNED,
                            MPI_ANY_SOURCE,
                            0,
                            MPI_COMM_WORLD,
                            &request)) < 0) {
            MPI_Abort(MPI_COMM_WORLD, rc);
            return rc;
        }

        return find_path(board, *init_pos, 1);
    } else {
        MPI_Status status;

        if ((rc = MPI_Recv(init_pos, sizeof(pos_t), MPI_BYTE, LEAD_PROC_ID, 0, MPI_COMM_WORLD, &status)) < 0) {
            MPI_Abort(MPI_COMM_WORLD, rc);
            return rc;
        }

        // printf("child proc with pid = %d init pos received: (%d, %d)\n", pid, pos.x, pos.y);
        return find_path(board, *init_pos, 1);
    }
}

int find_path_parallel(board_t *board, pos_t init_pos, int num_proc) {
    pos_t path[board->max_moves];
    path[0] = init_pos;
    int epochs = num_proc / POSSIBLE_MOVES_NUM + 1;
    int i = 0;
    int j;
    int rc = OK;

    if (pid == LEAD_PROC_ID) {
        printf("lead proc with pid = %d split task stage for %d processes\n", pid, num_proc);
        for (int proc_num = num_proc - 1, proc_id = 1; i < epochs; i++, proc_num -= POSSIBLE_MOVES_NUM) {
            pos_t next_pos;
            for (j = 0; j < proc_num && j < POSSIBLE_MOVES_NUM; j++, proc_id++) {
                if (move_possible(board, path[i], possible_moves[j], &next_pos)) {
                    pos_t buff[i + 2];

                    for (int k = 0; k <= i; k++) {
                        buff[k] = path[k];
                    }
                    buff[i + 1] = next_pos;

                    if ((rc = MPI_Send(buff, (i + 2) * sizeof(pos_t), MPI_BYTE, proc_id, 0, MPI_COMM_WORLD)) < 0) {
                        MPI_Abort(MPI_COMM_WORLD, rc);
                        return rc;
                    }
                }
            }
            path[i + 1] = next_pos;
        }
        path[i] = move(path[i - 1], possible_moves[j]);
        return find_path_continuation(board, path, i);
    } else {
        int count = pid / POSSIBLE_MOVES_NUM + 2;
        pos_t buff[count];
        MPI_Status status;

        printf("child proc with pid = %d receiving init path with size %d from lead proc;\n", pid, count);

        if ((rc = MPI_Recv(buff, count * sizeof(pos_t), MPI_BYTE, LEAD_PROC_ID, 0, MPI_COMM_WORLD, &status)) < 0) {
            MPI_Abort(MPI_COMM_WORLD, rc);
            return rc;
        }

        char msg[1000];
        char tmp[200];

        snprintf(tmp, 200, "child proc with pid = %d init path received:", pid);
        snprintf(msg, 1000, "%s", tmp);
        int offset = 0;

        for (int i = 0; i < count; i++) {
            offset += strlen(tmp);
            snprintf(tmp, 200, " (%d, %d)", buff[i].x, buff[i].y);
            snprintf(msg + offset, 1000 - offset, "%s", tmp);
        }

        printf("%s\n", msg);

        return find_path_continuation(board, buff, count);
    }
}

_Bool test_move_possible(board_t *board, pos_t pos, offset_t offset) {
    int x = ((int) pos.x) + offset.x_offset;
    int y = ((int) pos.y) + offset.y_offset;
    return (x >= 0 && y >= 0 && x < board->width && y < board->height);
}

_Bool path_is_correct(board_t *board) {
    int i, j;
    int value = 0;
    pos_t pos;

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


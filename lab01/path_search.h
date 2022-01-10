//
// Created by Karpukhin Aleksandr on 03.01.2022.
//

#ifndef LAB01_PATH_SEARCH_H
#define LAB01_PATH_SEARCH_H

#include "mpi.h"
#include "board.h"

extern int pid;
extern MPI_Request request;

_Bool move_possible(board_t *board, pos_t pos, offset_t offset);
pos_t move(pos_t pos, offset_t offset);
int find_path(board_t *board, pos_t pos, unsigned move_num);
int find_path_continuation(board_t *board, pos_t *path, size_t path_len);
int find_path_parallel(board_t *board, pos_t init_pos, int num_proc);
int find_path_parallel_greedy(board_t *board, int num_proc);
_Bool path_is_correct(board_t *board);

#endif //LAB01_PATH_SEARCH_H

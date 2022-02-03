//
// Created by Karpukhin Aleksandr on 03.01.2022.
//

#ifndef LAB01_PATH_SEARCH_H
#define LAB01_PATH_SEARCH_H

#include "mpi.h"
#include "board.h"

extern int pid;
extern int num_proc;

_Bool move_possible(board_t *board, pos_t pos, offset_t offset, pos_t *new_pos);

pos_t move(pos_t pos, offset_t offset);

err_code_t find_path(board_t *board, pos_t pos, unsigned move_num);

err_code_t find_path_parallel(board_t *board, pos_t *pos, unsigned task_num, unsigned move_idx);

_Bool path_is_correct(board_t *board);

#endif //LAB01_PATH_SEARCH_H

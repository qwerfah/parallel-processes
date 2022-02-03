//
// Created by Karpukhin Aleksandr on 31.01.2022.
//

#ifndef LAB01_TASK_H
#define LAB01_TASK_H

#define MAX_PROC_NUM  64
#define LONG_MSG_BUFF_SIZE 1000
#define SHORT_MSG_BUFF_SIZE 100

#include "board.h"

typedef struct task {
    unsigned pid;
    pos_t prefix[MAX_PROC_NUM / POSSIBLE_MOVES_NUM + 1];
    size_t prefix_length;
} task_t;

typedef struct batch {
    size_t count;
    task_t tasks[POSSIBLE_MOVES_NUM];
} batch_t;

typedef struct task_list_node {
    task_t task;
    struct task_list_node *next;
} task_list_node_t;

typedef struct task_list {
    task_list_node_t *head;
    size_t count;
} task_list_t;

typedef err_code_t (*path_search_t)(board_t *, pos_t, unsigned);

extern task_list_t list;

task_t task_ctr(unsigned pid, pos_t *prefix, size_t prefix_length);

batch_t batch_ctr(task_t *tasks, size_t count);

err_code_t add_task_to_list(task_t *task);

err_code_t try_pop_task_list_lead(task_t *task);

err_code_t schedule_tasks(void);

err_code_t accept_tasks(void);

err_code_t execute_tasks(board_t *board, path_search_t search);

err_code_t print_task(task_t *task);

#endif //LAB01_TASK_H

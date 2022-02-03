//
// Created by Karpukhin Aleksandr on 03.01.2022.
//

#ifndef LAB01_BOARD_H
#define LAB01_BOARD_H

#include <stdlib.h>
#include <stdio.h>

#include "defines.h"

typedef struct board board_t;

typedef struct pos {
    unsigned x;
    unsigned y;
} pos_t;

typedef struct offset {
    int x_offset;
    int y_offset;
} offset_t;

// Board operations structure
typedef struct board_ops {
    err_code_t (*init)(board_t *, struct board_ops, size_t, size_t);

    err_code_t (*dispose)(board_t *);

    int (*get)(board_t *, pos_t);

    err_code_t (*set)(board_t *, pos_t, unsigned);

    void (*print)(board_t *);

    err_code_t (*fprint)(board_t *, char *);
} board_ops_t;

// Board structure
struct board {
    unsigned *raw;
    size_t height;
    size_t width;
    size_t max_moves;
    board_ops_t ops;
};

board_t create(void);

board_ops_t default_board_ops(void);

#endif //LAB01_BOARD_H

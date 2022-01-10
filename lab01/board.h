//
// Created by Karpukhin Aleksandr on 03.01.2022.
//

#ifndef LAB01_BOARD_H
#define LAB01_BOARD_H

#include <stdlib.h>
#include <stdio.h>

#include "defines.h"

typedef struct board_t board_t;
typedef struct board_ops_t board_ops_t;
typedef struct pos_t pos_t;
typedef struct offset_t offset_t;

struct pos_t {
    unsigned x;
    unsigned y;
};

struct offset_t {
    int x_offset;
    int y_offset;
};

// Board operations structure
struct board_ops_t {
    int (*init)(board_t *, board_ops_t, size_t, size_t);

    int (*dispose)(board_t *);

    int (*get)(board_t *, pos_t);

    int (*set)(board_t *, pos_t, unsigned);

    void (*print)(board_t *);

    int (*fprint)(board_t *, char *);
};

// Board structure
struct board_t {
    unsigned *raw;
    size_t height;
    size_t width;
    size_t max_moves;
    board_ops_t ops;
};

board_t create(void);

board_ops_t default_board_ops(void);

#endif //LAB01_BOARD_H

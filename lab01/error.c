//
// Created by Karpukhin Aleksandr on 30.01.2022.
//

#include "error.h"
#include <stdio.h>

void error_code_handler(err_code_t code) {
    switch (code) {
        case ERR_NOT_ALLOCATED:
            printf(ERR_NOT_ALLOCATED_MESSAGE);
        case ERR_OUT_OF_RANGE:
            printf(ERR_OUT_OF_RANGE_MESSAGE);
        case ERR_OPEN_FILE_TO_WRITE:
            printf(ERR_OPEN_FILE_TO_WRITE_MESSAGE);
        case ERR_INVALID_ARGV:
            printf(ERR_INVALID_ARGV_MESSAGE);
        case ERR_NULL_POINTER:
            printf(ERR_NULL_POINTER_MESSAGE);
        case ERR_INVALID_LENGTH:
            printf(ERR_INVALID_LENGTH_MESSAGE);
        case ERR_INVALID_TASK_NUM:
            printf(ERR_INVALID_TASK_NUM_MESSAGE);
        case ERR_EMPTY_TASK_LIST:
            printf(ERR_EMPTY_TASK_LIST_MESSAGE);
        case ERR_UNKNOWN_CODE:
            printf(ERR_UNKNOWN_CODE_MESSAGE);
        default:
            printf(ERR_UNKNOWN_MESSAGE);
    }
}
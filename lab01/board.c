//
// Created by Karpukhin Aleksandr on 03.01.2022.
//

#include "board.h"

board_t create(void)
{
    board_t board = {NULL, -1, -1, -1, default_board_ops()};

    return board;
}

int init(board_t *board, board_ops_t ops, size_t height, size_t width)
{
    int rc = OK;
    size_t cells = height * width;

    board->ops.dispose(board);
    if ((board->raw = malloc(cells * sizeof(unsigned))))
    {
        board->height = height;
        board->width = width;
        board->max_moves = cells;
        board->ops = ops;
    }
    else
    {
        rc = ERR_NOT_ALLOCATED;
    }

    return rc;
}

int get(board_t *board, pos_t pos)
{
    if (!board->raw)
        return ERR_NOT_ALLOCATED;
    return (pos.x < board->width && pos.y < board->height)
               ? board->raw[pos.y * board->width + pos.x]
               : ERR_OUT_OF_RANGE;
}

int set(board_t *board, pos_t pos, unsigned value)
{
    int rc;

    if (!board->raw)
    {
        rc = ERR_NOT_ALLOCATED;
    }
    else if (pos.x >= board->width || pos.y >= board->height)
    {
        rc = ERR_OUT_OF_RANGE;
    }
    else
    {
        rc = (board->raw[pos.y * board->width + pos.x] = value);
    }

    return rc;
}

int dispose(board_t *board)
{
    int rc = OK;

    if (board->raw)
    {
        free(board->raw);
        board->raw = NULL;
    }
    else
    {
        rc = ERR_NOT_ALLOCATED;
    }

    return rc;
}

void sprint(FILE *stream, board_t *board)
{
    fprintf(stream, "\n|");
    for (int i = 0; i < board->width; i++)
    {
        fprintf(stream, "-----|");
    }
    for (int i = 0; i < board->height; i++)
    {
        fprintf(stream, "\n|");
        for (int j = 0; j < board->width; j++)
        {
            pos_t pos = {j, i};
            fprintf(stream, "%5u|", board->ops.get(board, pos));
        }
        fprintf(stream, "\n|");
        for (int i = 0; i < board->width; i++)
        {
            fprintf(stream, "-----|");
        }
    }
    fprintf(stream, "\n");
}

void print(board_t *board)
{
    sprint(stdout, board);
}

int fprint(board_t *board, char *filename)
{
    FILE *file;
    int rc = OK;

    if ((file = fopen(filename, "w")))
    {
        sprint(file, board);
        fclose(file);
    }
    else
    {
        rc = ERR_OPEN_FILE;
    }

    return rc;
}

// Construct default board operation structure with given operations implementations
board_ops_t default_board_ops(void)
{
    board_ops_t ops = {
        .init = init,
        .dispose = dispose,
        .get = get,
        .set = set,
        .print = print,
        .fprint = fprint};
    return ops;
}

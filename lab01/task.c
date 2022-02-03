//
// Created by Karpukhin Aleksandr on 31.01.2022.
//

#include "task.h"
#include "mpi.h"
#include <assert.h>
#include "string.h"
#include "path_search.h"

extern int num_proc;

task_list_t list = {NULL, 0};

task_t task_ctr(unsigned pid, pos_t *prefix, size_t prefix_length) {
    task_t task;

    task.pid = pid;
    task.prefix_length = prefix_length;
    memcpy(task.prefix, prefix, prefix_length * sizeof(pos_t));

    return task;
}

batch_t batch_ctr(task_t *tasks, size_t count) {
    batch_t batch;

    batch.count = count;
    memcpy(batch.tasks, tasks, count * sizeof(task_t));

    return batch;
}

err_code_t add_task_to_list(task_t *task) {
    if (task == NULL) {
        return ERR_NULL_POINTER;
    } else if (task->prefix_length < 0) {
        return ERR_INVALID_LENGTH;
    }

    task_list_node_t *node = (task_list_node_t *) calloc(1, sizeof(task_list_node_t));

    if (node == NULL) {
        return ERR_NOT_ALLOCATED;
    }

    node->task = *task;

    if (list.head == NULL) {
        list.head = node;
        list.head->next = NULL;
        list.count = 1;
    } else {
        task_list_node_t *head = list.head;
        list.head = node;
        list.head->next = head;
        list.count++;
    }

    return OK;
}

err_code_t try_pop_task_list_lead(task_t *task) {
    if (task == NULL) {
        return ERR_NULL_POINTER;
    } else if (list.head == NULL) {
        return ERR_EMPTY_TASK_LIST;
    }

    task_list_node_t *head = list.head;
    *task = head->task;
    list.head = head->next;
    list.count--;
    free(head);

    return OK;
}

err_code_t schedule_tasks(void) {
    assert(list.count >= num_proc);
    task_t *tasks[num_proc];
    unsigned task_counts[num_proc];
    int rc = OK;

    memset(task_counts, 0, num_proc * sizeof(unsigned));

    printf("\nLEAD: SCHEDULING TASKS, LIST_HEAD=%p LIST_COUNT=%lu\n", list.head, list.count);

    for (int i = 0; i < num_proc; i++) {
        if ((tasks[i] = calloc(list.count / num_proc + 1, sizeof(task_t))) == NULL) {
            for (int j = 0; j < i; j++) { free(tasks[j]); }
            return ERR_NOT_ALLOCATED;
        }
        printf("\nLEAD: ALLOCATE FOR PID=%d, TASK_COUNT=%d\n", i, task_counts[i]);
    }

    task_t task;

    while (try_pop_task_list_lead(&task) == OK) {
        tasks[task.pid][task_counts[task.pid]++] = task;
        printf("\nLEAD: REPLACING TASK WITH PID=%d, TASK_COUNT=%d\n", task.pid, task_counts[task.pid]);
    }

    printf("\nLEAD: SCHEDULED ");
    for (int i = 0; i < num_proc; i++) {
        printf(" FOR PID=%d - %d TASKS,", i, task_counts[i]);
    }
    printf("\n");

    for (int i = 1; i < num_proc; i++) {
        batch_t batch = batch_ctr(tasks[i], task_counts[i]);
        if ((rc = MPI_Send(&batch,
                           sizeof(batch_t),
                           MPI_BYTE,
                           i,
                           0,
                           MPI_COMM_WORLD)) != OK) {
            free(tasks[i]);
            MPI_Abort(MPI_COMM_WORLD, rc);
            return rc;
        }
        free(tasks[i]);
    }

    for (int i = 0; i < task_counts[0]; i++) {
        if ((rc = add_task_to_list(&tasks[0][i])) != OK) {
            return rc;
        }
    }

    free(tasks[0]);

    return OK;
}

err_code_t accept_tasks(void) {
    int rc = OK;
    batch_t batch;
    MPI_Status status;

    if ((rc = MPI_Recv(&batch,
                       sizeof(batch_t),
                       MPI_BYTE,
                       MPI_ANY_SOURCE,
                       0,
                       MPI_COMM_WORLD,
                       &status)) != OK) {
        MPI_Abort(MPI_COMM_WORLD, rc);
        return rc;
    }

    printf("\nPID=%d: %lu TASKS RECEIVED\n", pid, batch.count);

    for (int i = 0; i < batch.count; i++) {
        print_task(batch.tasks + i);
        if ((rc = add_task_to_list(&batch.tasks[i])) != OK) {
            return rc;
        }
    }

    return OK;
}

err_code_t execute_tasks(board_t *board, path_search_t search) {
    if (board == NULL || board->raw == NULL) {
        return ERR_NULL_POINTER;
    }

    task_t task;

    printf("\nPID=%d: EXECUTING TASKS", pid);

    while (try_pop_task_list_lead(&task) == OK) {
        assert(task.pid == pid);
        memset(board->raw, 0, board->max_moves * sizeof(unsigned));

        print_task(&task);

        int i;
        int rc;

        for (i = 0; i < task.prefix_length - 1; i++) {
            board->ops.set(board, task.prefix[i], i + 1);
        }

        printf("PID=%d: TASK INITED\n", pid);

        if ((rc = search(board, task.prefix[i], i + 1)) != PATH_NOT_FOUND) {
            printf("\nPID=%d: TASK EXITED WITH CODE %d\n", pid, rc);
            return rc;
        }

        printf("\nPID=%d: TASK EXITED WITH CODE %d\n", pid, rc);
    }

    return ERR_EMPTY_TASK_LIST;
}

err_code_t print_task(task_t *task) {
    if (task == NULL) {
        return ERR_NULL_POINTER;
    }

    char long_buff[LONG_MSG_BUFF_SIZE];
    char short_buff[SHORT_MSG_BUFF_SIZE];

    sprintf(long_buff, "\nPID=%d: TASK FOR PID=%d, PREFIX=[", pid, task->pid);

    unsigned long_offset = strlen(long_buff);

    for (int i = 0; i < task->prefix_length; i++) {
        memset(short_buff, 0, SHORT_MSG_BUFF_SIZE);
        sprintf(short_buff, " (%d, %d)", task->prefix[i].x, task->prefix[i].y);
        unsigned short_offset = strlen(short_buff);
        sprintf(long_buff + long_offset, "%s", short_buff);
        long_offset += short_offset;
    }

    sprintf(long_buff + long_offset, " ]\n");

    printf("%s", long_buff);

    return OK;
}

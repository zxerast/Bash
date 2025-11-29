#include "job_control.h"

JobList jobs = {NULL, NULL, 0};

Job *job_create(const char *cmd, pid_t pgid, int proc_count) {
    Job *j = calloc(1, sizeof(Job));
    if (!j) return NULL;

    j->id = 0;   // присвоим при вставке
    j->pgid = pgid;
    j->cmd = strdup(cmd);

    j->procs = calloc(proc_count, sizeof(Proc));
    j->proc_count = proc_count;

    j->stopped = 0;
    j->running = 1;     // job жив при создании
    j->background = 0;

    j->prev = j->next = NULL;
    return j;
}

void job_add(JobList *list, Job *j) {
    list->count++;
    j->id = list->count;

    if (list->tail == NULL) {
        list->head = list->tail = j;
        return;
    }

    list->tail->next = j;
    j->prev = list->tail;
    list->tail = j;
}

void job_remove(JobList *list, Job *j) {
    if (!j) return;

    if (j->prev)
        j->prev->next = j->next;
    else
        list->head = j->next;

    if (j->next)
        j->next->prev = j->prev;
    else
        list->tail = j->prev;

    list->count--;

    free(j->cmd);
    free(j->procs);
    free(j);
}

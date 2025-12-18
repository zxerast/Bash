#include "job_control.h"
#include <stdio.h>

JobList jobs = {NULL, NULL, 0}; // глобальный список job'ов

Job *job_create(const char *cmd, pid_t pgid, int proc_count) {
    Job *j = calloc(1, sizeof(Job));
    if (!j) return NULL;

    j->id = 0;   // присвоим при вставке
    j->pgid = pgid; // пид группы
    j->cmd = strdup(cmd);   // полная команда

    j->procs = calloc(proc_count, sizeof(Proc));    // массив процессов
    j->proc_count = proc_count; // количество процессов

    j->stopped = 0;     // флаги job'а
    j->running = 1;     // job жив при создании
    j->background = 0;

    j->prev = j->next = NULL;   // первому job'у одиноко в списке
    return j;
}

void job_add(JobList *list, Job *j) {   // поэтому добавим ему друзей
    list->count++;      
    j->id = list->count;    // нумеруем job

    if (list->tail == NULL) {
        list->head = list->tail = j;    // первый элемент списка
        return;
    }

    list->tail->next = j;
    j->prev = list->tail;
    list->tail = j;
}

void job_add_proc(Job *j, int index, pid_t pid) { // добавление процесса в job
    if (!j || index < 0 || index >= j->proc_count)  // проверка корректности индекса
        return;

    j->procs[index].pid = pid;
    j->procs[index].stopped = 0;
    j->procs[index].status = 0;
    j->procs[index].finished = 0;
}


void job_remove(JobList *list, Job *j) {    // удаляем job из списка
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

Job *find_job(JobList *list, int id) {  // поиск job по id
    Job *j = list->head;
    while (j) {
        if (j->id == id) return j;
        j = j->next;
    }
    return NULL;
}

int all_procs_stopped(Job *job) {
    for (int i = 0; i < job->proc_count; i++) {
        if (!job->procs[i].stopped)
            return 0;
    }
    return 1;
}

int all_procs_finished(Job *job) {
    for (int i = 0; i < job->proc_count; i++) {
        if (!job->procs[i].finished)
            return 0;
    }
    return 1;
}

int reap_background_jobs() {    // убираем завершившиеся фоновые job'ы
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) { // сначала обновляем статусы процессов
        Job *j = jobs.head;
        while (j != NULL) {
            for (int i = 0; i < j->proc_count; i++) {   // смотрим все процессы во всех job'ах
                if (j->procs[i].pid == pid) {
                    j->procs[i].status = status;

                    if (WIFEXITED(status) || WIFSIGNALED(status)) {
                        j->procs[i].finished = 1;
                        j->procs[i].stopped = 0;
                    }

                    else if (WIFSTOPPED(status)) {
                        j->procs[i].stopped = 1;       // обновляем флаги в зависимости от статуса
                        j->procs[i].finished = 0;
                    }

                    else if (WIFCONTINUED(status)) {
                        j->procs[i].stopped = 0;       // обновляем флаги в зависимости от статуса
                    }

                    int all_finished = 1;
                    int all_stopped  = 1;

                    for (int k = 0; k < j->proc_count; k++) {

                        if (!j->procs[k].finished)
                            all_finished = 0;

                        if (!j->procs[k].stopped)
                            all_stopped = 0;
                    }

                    if (all_finished) {
                        j->exited = 1;
                        j->stopped  = 0;
                    }

                    else if (all_stopped) {
                        j->stopped = 1;
                    }

                    else {
                        j->stopped  = 0;
                        j->exited = 0;
                    }


                    break;
                }
            }
            j = j->next;
        }
    }

    Job *j = jobs.head;
    while (j != NULL) {         // теперь удаляем завершившиеся job'ы
        Job *next = j->next;

        int all_finished = 1;
        for (int i = 0; i < j->proc_count; i++) {   // проверяем все процессы в job-е
            if (j->procs[i].finished != 1) {    // если хоть один не завершён — не удаляем
                all_finished = 0;
                break;
            }
        }

        if (all_finished) { // если все процессы завершены удаляем
            printf("\n[%d] Done           %s\n", j->id, j->cmd);
            job_remove(&jobs, j);
        }

        j = next;
    }

    return 0;
}

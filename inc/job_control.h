#pragma once

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

typedef struct proc {
    pid_t pid;        // PID процесса
    int   status;     // Последний известный статус (waitpid)
    int   exited;     // Флаг: процесс завершился?
    int   stopped;    // Флаг: процесс остановлен?
} Proc;

typedef struct job {
    int   id;              // Номер job-а: 1,2,3...
    pid_t pgid;            // Process group ID (лидер пайпа)

    char *cmd;             // Полная строка команды (для 'jobs')

    Proc *procs;           // Динамический массив процессов
    int   proc_count;      // Сколько процессов в job

    int   stopped;         // TRUE, если весь job остановлен
    int   running;         // TRUE, если хоть один процесс жив
    int   background;      // TRUE, если job запущен с `&`

    struct job *prev;      // Двусвязный список
    struct job *next;
} Job;

typedef struct job_list {
    Job *head;    // первый элемент
    Job *tail;    // последний элемент
    int  count;   // количество jobs
} JobList;

extern JobList jobs; 

Job *job_create(const char *cmd, pid_t pgid, int proc_count);
void job_add(JobList *list, Job *j);
void job_remove(JobList *list, Job *j);
#pragma once

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

typedef struct proc {   // структура одного процесса 
    pid_t pid;        
    int   status;     
    int   finished;    
    int   stopped;    
} Proc;

typedef struct job {
    int   id;              // номер job'а
    pid_t pgid;            // пид группы/лидера

    char *cmd;             // полная строка команды

    Proc *procs;           // массив процессов
    int   proc_count;      // количество процессов

    int   stopped;         // флаги состояния job'а
    int   running;         
    int   exited;         
    int   background;      

    struct job *prev;      
    struct job *next;
} Job;

typedef struct job_list {
    Job *head;    
    Job *tail;    
    int  count;   
} JobList;

extern JobList jobs; 

Job *job_create(const char *cmd, pid_t pgid, int proc_count);
void job_add(JobList *list, Job *j);
void job_remove(JobList *list, Job *j);
void job_add_proc(Job *j, int index, pid_t pid);
Job *find_job(JobList *list, int id);
int reap_background_jobs();
int all_procs_finished(Job *job);
int all_procs_stopped(Job *job);
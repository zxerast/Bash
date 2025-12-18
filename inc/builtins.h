#pragma once



#include <unistd.h>
#include <limits.h>
#include "ast.h"
#include "job_control.h"

typedef enum {
    NONE = 0, CD, EXIT,
    PWD, ECHO, HELP,
    HISTORY, JOBS, FG,
    BG, KILL, EXPORT
} Builtin;

Builtin is_builtin(ASTNode *node);
int builtin(ASTNode *node);
int builtin_cd(char **argv);
int builtin_exit(char **argv);
int builtin_pwd();
int builtin_echo(char **argv);
int builtin_help();
int builtin_history();
int builtin_jobs(JobList *jobs);
int builtin_bg(char **argv);
int builtin_fg(char **argv);
int builtin_kill(char **argv);
int builtin_export(char **argv);
int str2sig(const char *name);
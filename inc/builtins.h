#pragma once

#include <unistd.h>
#include <limits.h>
#include "ast.h"


int builtin(ASTNode *node);
int builtin_cd(char **argv);
int builtin_exit(char **argv);
int builtin_pwd();
int builtin_echo(char **argv);
int builtin_help();
int builtin_history();
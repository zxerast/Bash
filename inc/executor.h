#pragma once
#include "ast.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

int redirects(Redirect *r);
int exec_command(ASTNode *node, int is_pipe);
int exec_and_or(ASTNode *node);
int exec_seq(ASTNode *node);
int exec_pipe(ASTNode *node);
int execute(ASTNode *node, int stage);
int exitstatus(int status);
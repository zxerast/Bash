#pragma once
#include "ast.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

int redirects(Redirect *r);
int exec_command(ASTNode *node);
int exec_and_or(ASTNode *node);
int exec_seq(ASTNode *node);
int exec_background(ASTNode *node);
int exec_pipe(ASTNode *node);
int execute(ASTNode *node);
int exitstatus(int status);
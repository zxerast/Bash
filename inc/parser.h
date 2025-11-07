#pragma once

#include "ast.h"
#include "lists.h"

ASTNode *parse_sequence(Token *head);   // ;
ASTNode *parse_and_or(Token *head);     // &&, ||
ASTNode *parse_pipeline(Token *head);   // |
ASTNode *parse_command(Token *head);    // WORD, redirect
ASTNode *parse(Token *head);

void print_redirects(const Redirect *r, int indent);
void print_argv(char **argv, int indent);
void print_indent(int indent);
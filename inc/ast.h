#ifndef AST_H
#define AST_H
#include "lists.h"

typedef enum {  //  типы узлов AST
    NODE_COMMAND,
    NODE_PIPE,
    NODE_AND_OR,
    NODE_SEQ,
} NodeType;

typedef struct Redirect {   // структура перенаправлений в виде списка
    int type;                 // тип из TokenType
    char *filename;
    struct Redirect *next;
} Redirect;

typedef struct ASTNode {
    NodeType type;
    TokenType op;    // оператор (для PIPE, AND_IF, OR_IF, SEPARATOR)
    struct ASTNode *left;
    struct ASTNode *right;
    char **argv;              // аргументы команды
    Redirect *redirects;      // перенаправления
    int background;         // запущена ли команда в фоне
} ASTNode;

ASTNode *new_ast_node(NodeType type, TokenType op);
void free_ast(ASTNode *node);

#endif

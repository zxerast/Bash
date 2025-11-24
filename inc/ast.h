#ifndef AST_H
#define AST_H
#include "lists.h"

typedef enum {
    NODE_COMMAND,
    NODE_PIPE,
    NODE_AND_OR,
    NODE_SEQ,
} NodeType;

typedef struct Redirect {
    int type;                 // тип из TokenType
    char *filename;
    struct Redirect *next;
} Redirect;

typedef struct ASTNode {
    NodeType type;
    TokenType op;    
    struct ASTNode *left;
    struct ASTNode *right;
    char **argv;              // аргументы команды
    Redirect *redirects;      // перенаправления
} ASTNode;

// Функции создания и уничтожения узлов
ASTNode *new_ast_node(NodeType type, TokenType op);
void free_ast(ASTNode *node);

#endif

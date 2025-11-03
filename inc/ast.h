#ifndef AST_H
#define AST_H

typedef enum {
    NODE_COMMAND,
    NODE_PIPE,
    NODE_AND,
    NODE_OR,
    NODE_SEQ,
    NODE_BACKGROUND
} NodeType;

typedef struct Redirect {
    int type;                 // тип из TokenType
    char *filename;
    struct Redirect *next;
} Redirect;

typedef struct ASTNode {
    NodeType type;
    struct ASTNode *left;
    struct ASTNode *right;
    char **argv;              // аргументы команды
    Redirect *redirects;      // перенаправления
} ASTNode;

// Функции создания и уничтожения узлов
ASTNode *new_ast_node(NodeType type);
void free_ast(ASTNode *node);
void print_ast(ASTNode *node, int depth);

#endif

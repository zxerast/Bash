#include "ast.h"
#include <stdlib.h>
#include <stdio.h>

ASTNode *new_ast_node(NodeType type, TokenType op) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->op = op;
    node->type = type;
    return node;
}

void free_ast(ASTNode *node) {
    if (!node) return;
    free_ast(node->left);
    free_ast(node->right);
    if (node->argv) {
        for (int i = 0; node->argv[i]; i++)
            free(node->argv[i]);
        free(node->argv);
    }
    
    Redirect *r = node->redirects;
    while (r) {
        Redirect *next = r->next;
        free(r->filename);
        free(r);
        r = next;
    }
    free(node);
}

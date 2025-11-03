#include "ast.h"
#include <stdlib.h>
#include <stdio.h>

ASTNode *new_ast_node(NodeType type) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
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
    // очистка перенаправлений
    Redirect *r = node->redirects;
    while (r) {
        Redirect *next = r->next;
        free(r->filename);
        free(r);
        r = next;
    }
    free(node);
}

void print_ast(ASTNode *node, int depth) {
    if (!node) return;
    for (int i = 0; i < depth; i++) printf("  ");
    printf("Node type: %d\n", node->type);
    print_ast(node->left, depth + 1);
    print_ast(node->right, depth + 1);
}

#include "parser.h"

ASTNode *parse(Token *head)
{
    return parse_sequence(head);   // ;
}

ASTNode *parse_sequence(Token *head){
    Token *tmp = head;
    while (head->type != BACKGROUND && head->type != SEPARATOR){
        if(head->type == END) return parse_and_or(tmp);
        head = head->next;
    }
    ASTNode *node = new_ast_node(NODE_SEQ, head->type);
    head->type = END;
    node->left = parse_and_or(tmp);
    node->right = parse(head->next);
    return node;
}

ASTNode *parse_and_or(Token *head){
    return NULL;
}
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
    Token *tmp = head;
    while (head->type != AND_IF && head->type != OR_IF){
        if(head->type == END) return parse_pipeline(tmp);
        head = head->next;
    }
    ASTNode *node = new_ast_node(NODE_AND_OR, head->type);
    head->type = END;
    node->left = parse_pipeline(tmp);
    node->right = parse(head->next);
    return node;
}

ASTNode *parse_pipeline(Token *head){
    Token *tmp = head;
    while (head->type != PIPE && head->type != PIPE_AND){
        if(head->type == END) return parse_command(tmp);
        head = head->next;
    }
    ASTNode *node = new_ast_node(NODE_PIPE, head->type);
    head->type = END;
    node->left = parse_command(tmp);
    node->right = parse(head->next);
    return node;
}

ASTNode *parse_command(Token *head){
    ASTNode *node = new_ast_node(NODE_COMMAND, head->type);
    node->argv = NULL;
    node->redirects = NULL;

    size_t argc = 0;
    Token *tmp = head;

    while (tmp->type != END) {
        switch (tmp->type) {
            case WORD:
            case STRING: {
                node->argv = realloc(node->argv, sizeof(char *) * (argc + 2));
                node->argv[argc] = strdup(tmp->value);
                argc++;
                node->argv[argc] = NULL; // всегда завершаем NULL
                break;
            }

            case REDIRECT_IN:
            case REDIRECT_OUT:
            case REDIRECT_APPEND:
            case REDIRECT_ERR:
            case REDIRECT_FAR: {
                if (!tmp->next || tmp->next->type != WORD) {
                    fprintf(stderr, "Syntax error: expected filename after redirection\n");
                    free_ast(node);
                    return NULL;
                }

                Redirect *r = malloc(sizeof(Redirect));
                r->type = tmp->type;
                r->filename = strdup(tmp->next->value);
                r->next = NULL;

                // вставляем в конец списка перенаправлений
                if (!node->redirects) node->redirects = r;
                else {
                    Redirect *tmp = node->redirects;
                    while (tmp->next) tmp = tmp->next;
                    tmp->next = r;
                }

                tmp = tmp->next; // пропускаем имя файла
                break;
            }

            default:
                fprintf(stderr, "Syntax error near token type %d\n", tmp->type);
                free_ast(node);
                return NULL;
        }

        tmp = tmp->next;
    }

    // если нет аргументов — синтаксическая ошибка
    if (!node->argv || !node->argv[0]) {
        fprintf(stderr, "Syntax error: empty command\n");
        free_ast(node);
        return NULL;
    }
    return node;
}

// Печать дерева для отладки

void print_indent(int indent) {
    for (int i = 0; i < indent; i++)
        printf("  ");
}

void print_argv(char **argv, int indent) {
    if (!argv) return;
    print_indent(indent);
    printf("argv: ");
    for (int i = 0; argv[i]; i++)
        printf("\"%s\" ", argv[i]);
    printf("\n");
}

void print_redirects(const Redirect *r, int indent) {
    while (r) {
        print_indent(indent);
        printf("redirect: type=%d, file=\"%s\"\n", r->type, r->filename);
        r = r->next;
    }
}

void print_ast(ASTNode *node, int indent) {
    if (!node) return;

    print_indent(indent);
    switch (node->type) {
        case NODE_COMMAND:
            printf("NODE_COMMAND\n");
            print_argv(node->argv, indent + 1);
            print_redirects(node->redirects, indent + 1);
            break;

        case NODE_PIPE:
            printf("NODE_PIPE (operator: %d)\n", node->op);
            print_indent(indent + 1);
            printf("Left:\n");
            print_ast(node->left, indent + 2);
            print_indent(indent + 1);
            printf("Right:\n");
            print_ast(node->right, indent + 2);
            break;

        case NODE_AND_OR:
            printf("NODE_AND_OR (operator: %d)\n", node->op);
            print_indent(indent + 1);
            printf("Left:\n");
            print_ast(node->left, indent + 2);
            print_indent(indent + 1);
            printf("Right:\n");
            print_ast(node->right, indent + 2);
            break;

        case NODE_SEQ:
            printf("NODE_SEQ (operator: %d)\n", node->op);
            print_indent(indent + 1);
            printf("Left:\n");
            print_ast(node->left, indent + 2);
            print_indent(indent + 1);
            printf("Right:\n");
            print_ast(node->right, indent + 2);
            break;

        case NODE_BACKGROUND:
            printf("NODE_BACKGROUND\n");
            print_indent(indent + 1);
            printf("Left:\n");
            print_ast(node->left, indent + 2);
            print_indent(indent + 1);
            printf("Right:\n");
            print_ast(node->right, indent + 2);
            break;

        default:
            print_indent(indent);
            printf("Unknown node type: %d\n", node->type);
            break;
    }
} 
//    */

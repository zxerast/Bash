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
    node->left = NULL;
    node->right = NULL;
    node->argv = NULL;
    node->redirects = NULL;

    size_t argc = 0;
    Token *tmp = head;

    while (tmp->type != END) {  //  Создаём массив из слов которые будут образовывать команду
        switch (tmp->type) {
            case WORD:
            case STRING: {
                node->argv = realloc(node->argv, sizeof(char *) * (argc + 2));
                node->argv[argc] = strdup(tmp->value);
                argc++;
                node->argv[argc] = NULL; 
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
        free_ast(node);
        return NULL;
    }
    return node;
}





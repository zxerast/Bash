#include "parser.h"

ASTNode *parse(Token *head)
{
    ASTNode *pars = parse_sequence(head);
    if(pars != NULL){
        return parse_sequence(head);  
    }
    printf("Expected command before pipe\n");
    return NULL;
}

ASTNode *parse_sequence(Token *head){   // по приоритету начинаем вызов с самого низкого
    Token *tmp = head;
    while (head->type != BACKGROUND && head->type != SEPARATOR){    // пока не встретили ; или &
        if(head->type == END) return parse_and_or(tmp); // если не нашли, то парсим дальше по приоритету
        head = head->next;  // идём дальше по списку токенов
    }
    ASTNode *node = new_ast_node(NODE_SEQ, head->type); // создаём новый узел если нашли ; или &
    head->type = END;   // разделяем список токенов на две части END токеном в месте разделителя
    node->left = parse_and_or(tmp); // парсим в левой части следующую по приоритету часть
    node->right = parse(head->next); // в правой части дальше ищем возможный текущий приоритет
    return node;
}

ASTNode *parse_and_or(Token *head){       // Далее точно такая же логика, только для следующих приоритетов
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
    if (!node->left) return NULL;
    node->right = parse(head->next);
    if (!node->right) return NULL;
    return node;
}

ASTNode *parse_command(Token *head){
    ASTNode *node = new_ast_node(NODE_COMMAND, head->type); 
    node->left = NULL;  // команды не имеют потомков как самый верхний приоритет
    node->right = NULL;
    node->argv = NULL;
    node->redirects = NULL;

    size_t argc = 0;
    Token *tmp = head;

    while (tmp->type != END) {  // создаём массив из слов которые будут образовывать команду
        switch (tmp->type) {
            case WORD:
            case STRING: {
                node->argv = realloc(node->argv, sizeof(char *) * (argc + 2));
                node->argv[argc] = strdup(tmp->value);
                argc++;                    // слово или строка добавляются в аргументы команды
                node->argv[argc] = NULL; 
                break;
            }

            case REDIRECT_IN:
            case REDIRECT_OUT:
            case REDIRECT_APPEND:
            case REDIRECT_ERR:
            case REDIRECT_FAR: {
                if (!tmp->next || tmp->next->type != WORD) {    // после перенаправления должно идти имя файла
                    fprintf(stderr, "Syntax error: expected filename after redirection\n");
                    free_ast(node);
                    return NULL;
                }

                Redirect *r = malloc(sizeof(Redirect)); // создаём структуру перенаправления в виде списка
                r->type = tmp->type;    
                r->filename = strdup(tmp->next->value); 
                r->next = NULL;

                if (!node->redirects) node->redirects = r;  // добавляем в конец списка перенаправлений
                else {
                    Redirect *tmp = node->redirects;
                    while (tmp->next) tmp = tmp->next;  // иначе идём до конца списка и потом добавляем
                    tmp->next = r;
                }

                tmp = tmp->next; // пропускаем имя файла
                break;
            }

            default:
                fprintf(stderr, "Syntax error %d \n", tmp->type);
                free_ast(node);
                return NULL;
        }

        tmp = tmp->next;
    }

    if (!node->argv || !node->argv[0]) {
        free_ast(node);     // нет аргументов - нет и команды
        return NULL;
    }
    return node;
}





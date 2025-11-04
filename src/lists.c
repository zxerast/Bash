#include "lists.h"

Token *new_token(const char *text, TokenType type) {
    Token *t = malloc(sizeof(Token));
    if (!t) { perror("malloc"); exit(1); }
    t->value = strdup(text);   // копируем строку
    t->type = type;
    t->next = NULL;
    return t;
}

void append_token(Token **head, const char *text, TokenType type) {
    Token *t = new_token(text, type);
    if (*head == NULL) {
        *head = t;
    } else {
        Token *cur = *head;
        while (cur->next) cur = cur->next;
        cur->next = t;
    }
}

void free_tokens(Token *head) {
    while (head) {
        Token *tmp = head;
        head = head->next;
        free(tmp->value);
        free(tmp);
    }
}

void print_tokens(Token *head) {
    const char *types[] = {
        "WORD", "PIPE", "PIPE_AND", "REDIRECT_IN", "REDIRECT_OUT",
        "REDIRECT_APPEND", "REDIRECT_ERR", "REDIRECT_FAR", "BACKGROUND",
        "SEPARATOR", "AND_IF", "OR_IF", "STRING", "END"
    };

    Token *cur = head;
    while (cur) {
        printf("TOKEN { type = %s, value = \"%s\" }\n",
        types[cur->type], cur->value);
        cur = cur->next;
    }
}


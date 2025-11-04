#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef enum {
	WORD,			//Команда
	PIPE,			// |
	PIPE_AND,		// |&
	REDIRECT_IN,		// <
	REDIRECT_OUT,		// >
	REDIRECT_APPEND, 	// >>
	REDIRECT_ERR,		// &>
	REDIRECT_FAR,		// &>>
	BACKGROUND,		// &
	SEPARATOR,		// ;
	AND_IF,			// &&
	OR_IF,			// ||
	STRING,			// "Строка"
    END	
} TokenType;

typedef struct Token {
    char *value;          // сам текст токена
    TokenType type;
    struct Token *next;   // ссылка на следующий
} Token;

Token *new_token(const char *text, TokenType type);
void append_token(Token **head, const char *text, TokenType type);
void free_tokens(Token *head);
void print_tokens(Token *head);


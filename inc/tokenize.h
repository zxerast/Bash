#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lists.h"

void append(Token **t, char *buf, int j, TokenType type){
	if (j > 0) {
        	buf[j] = '\0';
        	append_token(t, buf, type);
    	}
}

Token *tokenize(char *line){
 Token *t = NULL;
 size_t len = strlen(line);
 char *buf = malloc(len + 1);	//	Наш буфер
 if(buf == NULL){
	perror("No memory");
 	exit(1);
 }
 size_t i = 0, j = 0;
 
 while(i < len){
 	while(i < len && line[i] == ' '){	//	Пропускаем пробелы
 		append(&t, buf, j, WORD);
		j = 0;
 		i++;
 	}

	if (line[i] == '\n') {	// По идее лишнее, но пока пусть стоит
    		i++;
    		continue;
	}

 	if(line[i] == '#')	break;	// Игнор комментария

	if(line[i] == '|'){
		append(&t, buf, j, WORD);
		j = 0;
		buf[j++] = line[i++];
		if(line[i] == '|'){
			buf[j++] = line[i++];
 			append(&t, buf, j, OR_IF);
		}
		else if(line[i] == '&'){
			buf[j++] = line[i++];
 			append(&t, buf, j, PIPE_AND);
		}
		else {
			append(&t, buf, j, PIPE);
		}
		j = 0;
		continue;	
	}

	if(line[i] == '>'){
		append(&t, buf, j, WORD);
		j = 0;
		buf[j++] = line[i++];
		if(line[i] == '>'){
			buf[j++] = line[i++];
 			append(&t, buf, j, REDIRECT_APPEND);
		}
		else {
			append(&t, buf, j, REDIRECT_OUT);
		}
		j = 0;
		continue;
	}

	if(line[i] == '<'){
		append(&t, buf, j, WORD);
		j = 0;
		buf[j++] = line[i++];
		append(&t, buf, j, REDIRECT_IN);
		j = 0;
		continue;	
	}
	
	if(line[i] == ';'){
		append(&t, buf, j, WORD);
		j = 0;
		buf[j++] = line[i++];
		append(&t, buf, j, SEPARATOR);
		j = 0;
		continue;	
	}

	if(line[i] == '&'){
		append(&t, buf, j, WORD);
		j = 0;
		buf[j++] = line[i++];
		if(line[i] == '&'){
			buf[j++] = line[i++];
			append(&t, buf, j, AND_IF);
		}
		else if(line[i] == '>'){
			buf[j++] = line[i++];
			if(line[i] == '>'){
				buf[j++] = line[i++];
				append(&t, buf, j, REDIRECT_FAR);
			}
			else{
				append(&t, buf, j, REDIRECT_ERR);
			}
		}
		else{
			buf[j] = '\0';
			append(&t, buf, j, BACKGROUND);
		}
		j = 0;
		continue;	
	}

 	if(line[i] == '\\'){
 		if(i + 1 < len){
 			buf[j++] = line[i + 1]; // экранирование
			i += 2;
			continue;
 		}
		else{
 			buf[j++] = line[i++]; // экранирование
			continue;
		}	
 	}

 	if(line[i] == '"'){
 		append(&t, buf, j, WORD);
		j = 0;
		i++;
 		while(line[i] != '"'){
 			if(line[i] == '\\'){
 				if(i + 1 < len){
 					buf[j++] = line[++i]; // экранирование
 					i++;
 					continue;
 				}	
 			}

 			buf[j++] = line[i++];	//	Записываем символы в кавычках как они есть, не пропуская пробелы
 		}
 		i++;
 		append(&t, buf, j, STRING);
		j = 0;
 		continue;	
 	}	

 	if(line[i] == '\''){
 		append(&t, buf, j, WORD);
		j = 0;
		i++;
 		while(line[i] != '\''){
 			buf[j++] = line[i++];	//	Записываем символы в кавычках как они есть, не пропуская пробелы
 		}
 		i++;
 		append(&t, buf, j, STRING);
		j = 0;
 		continue;	
 	}	


 	if (i >= len) break;	// Вышли за пределы строки
 	buf[j++] = line[i++];	//	Записываем символы в буфер(слово)
 }
 
 if(j > 0){
 	append(&t, buf, j, WORD);
	j = 0;
 }
 
 append_token(&t, "", END);
 
 free(buf);
 return t;
}

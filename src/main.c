#include "prompt.h"
#include "lists.h"
#include "tokenize.h"
#include "input.h"

int main(){

    while (1) {
        print_prompt();               //	вывод приглашения

        char *line = read_line();     //	считываем строку с учётом кавычек
        if (!line)                    //	ctrl+d
            break;

        if (*line == '\0') {          //    	пустая строка
            free(line);
            continue;
        }

        Token *tokens = tokenize(line); //	токенизация
        free(line);

        print_tokens(tokens);          //	вывод
        free_tokens(tokens);         
    }
    return 0;
}

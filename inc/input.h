#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#define HISTORY_FILE ".my_shell_history"

char *full_command = NULL;

char *read_line(char *prompt) {
    int open_quote = 0;
    int open_squote = 0;
    int last_slash = 0;
    char *line = NULL;      // начальный буфер
    ssize_t nread = 0;
    char *full_line = NULL; // накопленная строка
    
    while (1) {
        line = readline(prompt);
        prompt = NULL;

        if (!line) { // EOF
            free(full_line);
            return NULL;
        }

        nread = strlen(line);

        last_slash = 0;
        if (nread > 0 && line[nread - 1] == '\\') {
            last_slash = 1;
            line[nread - 1] = '\0'; // убираем слэш 
            nread--;
        } 

        for (ssize_t i = 0; i < nread; i++){
            if (line[i] == '\\') {       
                if(i + 1 < nread){         
                    i++; // пропускаем экранированный символ
                    continue;
                }
            }
         
            if (line[i] == '"' && !open_squote){
                open_quote = !open_quote;
            }
            else if (line[i] == '\'' && !open_quote)
                open_squote = !open_squote;
        }

        // добавляем в накопитель
        size_t old_len = full_line ? strlen(full_line) : 0;	// если старая строка пустая, то берём за старый размер 0 иначе размер строки
        full_line = realloc(full_line, old_len + nread + 2);	
        memcpy(full_line + old_len, line, nread + 1);		// переносим
        full_line[old_len + nread] = last_slash ? '\0' : '\n';
        full_line[old_len + nread + 1] = '\0';

        if (!open_quote && !open_squote && !last_slash){
            last_slash = 0;
            if (nread > 0 && line[0] != '\0') {
                add_history(line); // add to history
            }
            break; // если кавычки закрылись — выходим
        }

        fprintf(stderr, "> "); // продолжаем ввод
    }
    
    if (full_command) {
        free(full_command);
    }   
    
    full_command = malloc(strlen(full_line) + 1);
    strcpy(full_command, full_line);

    free(line);
    return full_line;
}

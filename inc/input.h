#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *read_line(void) {
    int open_quote = 0;
    int open_squote = 0;
    int last_slash = 0;
    char *line = NULL;      // начальный буфер
    size_t len = 0;         // размер буфера
    ssize_t nread;
    char *full_line = NULL; // накопленная строка
    
    while (1) {
        nread = getline(&line, &len, stdin);
        if (nread == -1) { // EOF
            free(line);
            free(full_line);
            return NULL;
        }

        last_slash = 0;
        if (nread > 1 && line[nread - 2] == '\\' && line[nread - 1] == '\n') {
            last_slash = 1;
            line[nread - 2] = '\0'; // убираем слэш 
            nread -= 2;
        } 
        else if (line[nread - 1] == '\n') {
            line[nread - 1] = '\0';
            nread--;
        }

        for (ssize_t i = 0; i < nread; i++){
            if (line[i] == '\\' && open_quote) {                
                i++; // пропускаем экранированный символ
                continue;
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
            break; // если кавычки закрылись — выходим
        }

        fprintf(stderr, "> "); // продолжаем ввод
    }

    free(line);
    return full_line;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

struct termios orig, raw;

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig);
    raw = orig;

    raw.c_lflag &= ~(ICANON | ECHO);  // не канонический режим и отключаем echo
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}

ssize_t my_getline(char **line, size_t *len) {
    enable_raw_mode();

    if (*line == NULL || *len == 0) {
        *len = 128;
        *line = malloc(*len);
    }

    size_t cap = 0;
    size_t cursor = 0;    // позиция курсора внутри строки

    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1) {
            disable_raw_mode();
            return -1;
        }

        // ENTER
        if (c == '\n' || c == '\r') {
            write(STDOUT_FILENO, "\n", 1);
            (*line)[cap] = '\0';
            disable_raw_mode();
            return cap;
        }

        // BACKSPACE
        if (c == 127 || c == '\b') {
            if (cursor > 0) {
                // сдвиг влево
                memmove(*line + cursor - 1, *line + cursor, len - cursor);
                cursor--;
                len--;

                write(STDOUT_FILENO, "\b", 1);
                write(STDOUT_FILENO, *line + cursor, len - cursor);
                write(STDOUT_FILENO, " ", 1);

                for (size_t i = cursor; i <= len; i++)
                    write(STDOUT_FILENO, "\b", 1);
            }
            continue;
        }

        // ESC-последовательности (стрелки)
        if (c == '\033') {
            char seq[2];
            if (read(STDIN_FILENO, &seq, 2) == 2) {
                if (seq[0] == '[') {
                    // LEFT
                    if (seq[1] == 'D' && cursor > 0) {
                        write(STDOUT_FILENO, "\033[D", 3);
                        cursor--;
                    }
                    // RIGHT
                    if (seq[1] == 'C' && cursor < len) {
                        write(STDOUT_FILENO, "\033[C", 3);
                        cursor++;
                    }
                }
            }
            continue;
        }

        // Добавление обычного символа
        if (len + 2 > *len) {
            *len *= 2;
            *line = realloc(*line, *len);
        }

        // вставка в середину
        if (cursor < len)
            memmove(*line + cursor + 1, *line + cursor, len - cursor);

        (*line)[cursor] = c;
        cursor++;
        len++;

        // печатаем с учётом вставки
        write(STDOUT_FILENO, &c, 1);
        write(STDOUT_FILENO, *line + cursor, len - cursor);

        // возвращаем курсор назад
        for (size_t i = cursor; i < len; i++)
            write(STDOUT_FILENO, "\033[D", 3);
    }
}


char *read_line(void) {
    int open_quote = 0;
    int open_squote = 0;
    int last_slash = 0;
    char *line = NULL;      // начальный буфер
    size_t len = 0;         // размер буфера
    ssize_t nread = 0;
    char *full_line = NULL; // накопленная строка
    
    while (1) {
        nread = my_getline(&line, &len);
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



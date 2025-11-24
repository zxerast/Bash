#include "prompt.h"
#include "lists.h"
#include "tokenize.h"
#include "input.h"
#include "parser.h"
#include "executor.h"

int main(){
    using_history();
    read_history(HISTORY_FILE);

    while (1) {
        char *line = read_line(create_prompt());     //	считываем строку с учётом кавычек
        if (!line)                    //	ctrl+d
            break;

        if (*line == '\0') {          //    	пустая строка
            free(line);
            continue;
        }

        Token *tokens = tokenize(line); //	токенизация
        ASTNode *root = parse(tokens);
        execute(root, 0);

        free(line);

        //print_tokens(tokens);          //	вывод
        free_tokens(tokens);         
    }

    write_history(HISTORY_FILE);

    return 0;
}

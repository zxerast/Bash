#include "prompt.h"
#include "lists.h"
#include "tokenize.h"
#include "input.h"
#include "parser.h"
#include "executor.h"
#include "job_control.h"


int main(){
// идентификатор shell'а
    pid_t shell_pid = getpid();

    setpgid(shell_pid, shell_pid);      // сделать шелл лидером своей группы
    tcsetpgrp(STDIN_FILENO, shell_pid); // передать терминал шеллу

// игнорировать SIGTTIN и SIGTTOU, чтобы шелл не стопился
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    int can_exit = 0;

    using_history();
    read_history(HISTORY_FILE);
          
    while (1) {
        char *line = read_line(create_prompt());     //	считываем строку с учётом кавычек
        if (!line){                    //	ctrl+d
            if (jobs.count > 0 && !can_exit){
                can_exit = 1;
                printf("exit\nThere are stopped jobs\n");
                continue;
            }
            break;
        }

        if (*line == '\0') {          //    	пустая строка
            free(line);
            continue;
        }

        Token *tokens = tokenize(line); //	токенизация
        ASTNode *root = parse(tokens);
        execute(root);

        free(line);
        free_ast(root);
        free_tokens(tokens);         
    }

    write_history(HISTORY_FILE);

    return 0;
}

#include "executor.h"
#include "builtins.h"
#include "job_control.h"
#include "variables.h"

extern char *full_command;  // наша полная команда

int execute(ASTNode *node) {    
    if (!node) return 0;

    switch (node->type) {   // в зависимости от типа узла вызываем соответствующую функцию которая рекурсивно обрабатывает дерево

        case NODE_COMMAND:
            for (int i = 0; node->argv[i] != NULL; i++) {
                char *old = node->argv[i];
                node->argv[i] = expand_var(old);
                free(old);
            }
            return exec_command(node);  // выполнение простой команды

        case NODE_PIPE:
            return exec_pipe(node); // выполнение пайпа

        case NODE_AND_OR:
           return exec_and_or(node); // выполнение логических AND/OR

        case NODE_SEQ:
            return exec_seq(node);  // выполнение последовательности команд и background
    }
    return 0;
}

int redirects(Redirect *r) {    
    while (r) {
        int file;

        switch (r->type) {  // тип редиректа
            case REDIRECT_IN:
                file = open(r->filename, O_RDONLY);
                if (file < 0) { 
                    perror("open <"); 
                    return -1; 
                }
                dup2(file, STDIN_FILENO);   // перенаправляем stdin(ввод) на файл
                close(file);
                break;

            case REDIRECT_OUT:
                file = open(r->filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (file < 0) { 
                    perror("open >"); 
                    return -1;
                }
                dup2(file, STDOUT_FILENO);  // перенаправляем stdout(вывод) на файл
                close(file);
                break;

            case REDIRECT_APPEND:
                file = open(r->filename, O_WRONLY | O_CREAT | O_APPEND, 0644); // открываем файл в режиме добавления, а не перезаписи как в прошлом случае
                if (file < 0) { 
                    perror("open >>"); 
                    return -1; 
                }
                dup2(file, STDOUT_FILENO);
                close(file);
                break;

            case REDIRECT_ERR:
                file = open(r->filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (file < 0) { 
                    perror("open &>"); 
                    return -1; 
                }
                dup2(file, STDERR_FILENO);  // перенаправляем stderr(вывод ошибок) на файл
                close(file);
                break;

            default:
                fprintf(stderr, "Unsupported redirect type\n");
                return -1;
        }

        r = r->next;
    }
    return 0;
}


int exec_command(ASTNode *node) {

    if (is_builtin(node) != NONE && node->background == 0) {    // если команда встроенная и не фоновая
        int saved_stdin  = dup(STDIN_FILENO);   // сохраняем дескрипторы 
        int saved_stdout = dup(STDOUT_FILENO);
        int saved_stderr = dup(STDERR_FILENO);

        if (saved_stdin < 0 || saved_stdout < 0 || saved_stderr < 0) {
            perror("dup");
            return 1;
        }

        int res;

        if (redirects(node->redirects) < 0) {   // выполняем редиректы
            res = 1;
        } 
        
        else {
            res = builtin(node);    // выполняем встроенную команду
        }

        dup2(saved_stdin,  STDIN_FILENO);   // восстанавливаем и закрываем дескрипторы
        dup2(saved_stdout, STDOUT_FILENO);
        dup2(saved_stderr, STDERR_FILENO);

        close(saved_stdin);
        close(saved_stdout);
        close(saved_stderr);

        return res;
    }

    

    pid_t pid = fork(); // если команда не встроенная — создаём для неё дочерний процесс
    
    if(pid < 0){
		perror ("fork");
		return 1;
	} 
/*
    Job *job = job_create(full_command, pid, 1);
    job->running = 1;
    job_add_proc(job, 0, pid);
*/

    if(pid == 0){
        setpgid(0, 0);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);

        if (redirects(node->redirects) < 0){    // перенаправления
            exit(1);	
        }

        execvp(node->argv[0], node->argv);  // выполняем команду
		perror("execvp");
		exit(1);
	}	

    setpgid(pid, pid); // в родителе устанавливаем группу процессов для дочернего процесса
    if (node->background == 0) {
        if (tcsetpgrp(STDIN_FILENO, pid) < 0) { // передаём терминал только foreground группе
            perror("tcsetpgrp");
        }
    }

    if(node->background == 0){
        int status;
        waitpid(pid, &status, WUNTRACED);   // ждем завершения или остановки если это не фон

        if (WIFSTOPPED(status)){    // остановка по ctrl+z(SIGTSTP)
            if (tcsetpgrp(STDIN_FILENO, getpid()) < 0) {  // возвращаем управление терминалом родительскому процессу
                perror("tcsetpgrp");
            }
            Job *job = job_create(full_command, pid, 1);
            job_add_proc(job, 0, pid);       
            job->stopped = 1;                   // создаём job, добавляем процессы, ставим флаги
            job->running = 0;                   // и добавляем в список jobs
	        job_add(&jobs, job);
            printf("\n[%d] Stopped         %s", job->id, job->cmd); // сообщение об остановке
            return 0;
        }
        

        if (tcsetpgrp(STDIN_FILENO, getpid()) < 0) {  // возвращаем терминал если дождались
            perror("tcsetpgrp");
        }

        if (WIFSIGNALED(status)) {
            return 128 + WTERMSIG(status);  // завершение по сигналу, возвращаем код результата + 128, как в bash
        }

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status); // если нормальное завершение, также возвращаем код выхода
        }

    }
    else {
        Job *job = job_create(full_command, pid, 1);   // идём сюда если фон 
        job_add_proc(job, 0, pid);
	    job_add(&jobs, job);
        printf("[%d] %d\n", job->id, pid);  // без ожидания, просто сообщение о том что он есть
        if (tcsetpgrp(STDIN_FILENO, getpid()) < 0) {  // передача терминала
            perror("tcsetpgrp");
        }
        return 0;
    }
    return 0;
}

int count_pipes(ASTNode *node) {    // если у нас много пайпов, считаем их количество
    int count = 0;
    while (node->type == NODE_PIPE) {
        if(node->right == NULL) return -1;
        count++;
        node = node->right;
    }
    return count + 1; 
}

void create_pipeline(ASTNode *node, ASTNode **arr, TokenType *op) { // а здесь их вместе собираем
    int n = 0;

    while (node->type == NODE_PIPE) {
        op[n] = node->op;
        arr[n++] = node->left;
        node = node->right;
    }
    arr[n++] = node;
}

int exec_pipe(ASTNode *node){
    int n = count_pipes(node);  // количество пайпов
    if(n < 0) return -1;
    ASTNode **cmds = malloc(n * sizeof(ASTNode*));  
    TokenType *op = malloc(n * sizeof(TokenType));
    create_pipeline(node, cmds, op);  // получаем массив пайпов

    int pipefd[2];  // файловые дескрипторы для пайпа
    int prev_fd = -1;   // самый первый stdin отсутствует

    int in_b = 0;          // далее мы разбиваем полную команду на часть с пайпами и без них
    int in_s = 0;          // учитывая кавычки

    pid_t pgid = 0;
    
    int len = strlen(full_command);
    char *tmp = NULL;

    for (int i = 0; i < len; i++){
        if (full_command[i] == '"'){
            if (in_b > 0) in_b--;
            else in_b++;    
            continue;
        }
        
        if (full_command[i] == '\''){
            if (in_s > 0) in_s--;
            else in_s++;      
            continue;
        }
    
        if(full_command[i] == '&' && in_b == 0 && in_s == 0){
            tmp = malloc(i + 1);
            memcpy(tmp, full_command, i);
            tmp[i] = '\0';
            memmove(full_command, full_command + i + 1, len - i - 1);
            full_command[len - i - 1] = '\0';
            break;  // в tmp теперь хранится часть команды до &
        }
    }
    
    if (tmp == NULL){
        tmp = strdup(full_command); // на случай если в команде только пайпы
    }
    

    Job *job = job_create(tmp, pgid, n);    
	job_add(&jobs, job);

    for (int i = 0; i < n; i++) {   // создаём процессы для каждого элемента пайпа через цикл
        if (i < n - 1) {    
            if (pipe(pipefd) < 0) { // создаём пайп если это не последний процесс
                perror("pipe");
                exit(1);
            }
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            if (pgid == 0)
                pgid = getpid();          // первая команда пайплайна становится лидером
            setpgid(0, pgid);

            signal(SIGTTOU, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGINT, SIG_DFL);
            signal(SIGCHLD, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTERM, SIG_DFL);

            if (prev_fd != -1) {    // если это не первый процесс, перенаправляем ввод
                dup2(prev_fd, STDIN_FILENO);
            }

            if (i < n - 1) {    // если он не последний, перенаправляем вывод
                dup2(pipefd[1], STDOUT_FILENO);
               
                if (op[i] == PIPE_AND) {    // ну и если это |&, то и ошибки тоже
                    dup2(pipefd[1], STDERR_FILENO);
                }
            }

            if (prev_fd != -1) close(prev_fd);  // закрываем старый ввод

            if (i < n - 1) {    // закрываем не нужные концы пайпа
                close(pipefd[0]);
                close(pipefd[1]);
            }

            if (redirects(cmds[i]->redirects) < 0){ // перенаправления
                exit(1);	
            }

            if (is_builtin(cmds[i]) != NONE){   
                int code = builtin(cmds[i]);  // если встройка
                exit(code);
            }

            execvp(cmds[i]->argv[0], cmds[i]->argv);
            perror("execvp");
            exit(1);
        }

        if (pgid == 0){   // в родителе устанавливаем pgid для всех процессов 
            pgid = pid;
            job->pgid = pgid;
        }
        setpgid(pid, pgid);

        if (prev_fd != -1){ // закрываем старый ввод в родителе
            close(prev_fd);
        }

        if (i < n - 1) {    // и вывод тоже
            close(pipefd[1]);
            prev_fd = pipefd[0];
        }

        job_add_proc(job, i, pid);    // добавляем новосозданный процесс в job

    }

    if (node->background == 1) {    // не ждём в фоне
        printf("[%d] %d\n", job->id, pgid);
        return 0;
    }
    else{
        tcsetpgrp(STDIN_FILENO, pgid);  // после создания всех процессов передаем терминал группе процессов
    }

    while (1) {   // пока есть хоть один живой процесс в job-е
        int status;
        pid_t w = waitpid(-pgid, &status, WUNTRACED); // ждём

        if (w == -1) break; // не дождались(

        if (WIFSTOPPED(status)) {   // встали
            for (int i = 0; i < job->proc_count; i++) { // смотрим, кто же встал
                if (job->procs[i].pid == w) {
                    job->procs[i].stopped = 1;  // и у того меняем флаг остановки
                    break;
                }
            }
        }

        if (WIFEXITED(status) || WIFSIGNALED(status)) { // завершились либо сдохли
            for (int i = 0; i < job->proc_count; i++) { //  также ищем виновника торжества 
                if (job->procs[i].pid == w) {
                    job->procs[i].finished = 1; //  и ему клеймо мертвеца 
                    break;
                }
            }
        }

        if (all_procs_stopped(job)) {
            job->stopped = 1;

            free(cmds);
            free(op);
            free(tmp);

            tcsetpgrp(STDIN_FILENO, getpid());
            printf("\n[%d] Stopped         %s\n", job->id, job->cmd);
            return 0;
        }
        
        if (all_procs_finished(job)) {
            job->exited = 1;

            free(cmds);
            free(op);
            free(tmp);

            tcsetpgrp(STDIN_FILENO, getpid());
            job_remove(&jobs, job);   // удаляем job
            return 0;
        }
    }
    return 0;
}

int exec_and_or(ASTNode *node){
    int status = execute(node->left);   // сперва выполняем левую часть

    if (node->op == AND_IF) {     
        if (status == 0)          
            return execute(node->right);
        else
            return status;        
    }                              // и в зависимости от результата и оператора выполняем правую часть или нет

    if (node->op == OR_IF) {      
        if (status != 0)          
            return execute(node->right);
        else
            return status;        
    }

    return status;
}

int exec_seq(ASTNode *node){
    if(node->op == SEPARATOR){  // если это ; то просто выполняем последовательно
        execute(node->left);
        return execute(node->right);
    }

    if(node->op == BACKGROUND){
        if(node->left){  
            node->left->background = 1; // ставим флаг background
            execute(node->left);
        }
        
        if(node->right){  
            node->right->background = 0;    // то что слева от & в фоне, а справа нет
            return execute(node->right);
        }
        return 0;
    }
    return 0;
}


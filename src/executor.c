#include "executor.h"
#include "builtins.h"
#include "job_control.h"

extern char *full_command;

int execute(ASTNode *node) {

    
    if (!node) return 0;

    if (node->left) node->left->background = node->background;

    if (node->right) node->right->background = node->background;

    switch (node->type) {

        case NODE_COMMAND:
            return exec_command(node);

        case NODE_PIPE:
            return exec_pipe(node);

        case NODE_AND_OR:
           return exec_and_or(node);

        case NODE_SEQ:
            return exec_seq(node);
    }
    return 0;
}

int redirects(Redirect *r) {
    while (r) {
        int file;

        switch (r->type) {
            case REDIRECT_IN:
                file = open(r->filename, O_RDONLY);
                if (file < 0) { 
                    perror("open <"); 
                    return -1; 
                }
                dup2(file, STDIN_FILENO);
                close(file);
                break;

            case REDIRECT_OUT:
                file = open(r->filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (file < 0) { 
                    perror("open >"); 
                    return -1;
                }
                dup2(file, STDOUT_FILENO);
                close(file);
                break;

            case REDIRECT_APPEND:
                file = open(r->filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
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
                dup2(file, STDERR_FILENO);
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


int exec_command(ASTNode *node){
    
    int cmd = builtin(node);
    
    if(cmd == 0){   // встроенная команда выполнена в родительском процессе
        return cmd;
    }


    pid_t pid = fork();
	job_add(&jobs, job_create(full_command, pid, 1));

	if(pid < 0){
		perror ("fork");
		return 1;
	} 

    if(pid == 0){
        setpgid(0, 0);
        signal(SIGINT, SIG_DFL);    // В дочернем процессе восстанавливаем сигналы по умолчанию
        signal(SIGTSTP, SIG_DFL);

        tcsetpgrp(STDIN_FILENO, getpid());  // Передаем управление терминалом дочернему процессу

        if (redirects(node->redirects) < 0){
            exit(1);	
        }

        execvp(node->argv[0], node->argv);
		perror("execvp");
		exit(1);
	}	

    setpgid(pid, pid); // Устанавливаем группу процессов для дочернего процесса

    signal(SIGINT, SIG_IGN);    // В родительском процессе игнорируем сигналы
    signal(SIGTSTP, SIG_IGN);

    int status;
    waitpid(pid, &status, WUNTRACED);

    if (WIFSTOPPED(status)){
        tcsetpgrp(STDIN_FILENO, getpid());  // Возвращаем управление терминалом родительскому процессу
        jobs.tail->stopped = 1;
        printf("\n[%d] Stopped         %s", jobs.tail->id, jobs.tail->cmd);
        return 0;
    }
    

    tcsetpgrp(STDIN_FILENO, getpid());  // Возвращаем управление терминалом родительскому процессу

    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return 1;
}

// Собираем команды пайпа в массив
int create_pipeline(ASTNode *node, ASTNode **arr) {
    int n = 0;

    while (node->type == NODE_PIPE) {
        arr[n++] = node->left;
        node = node->right;
    }

    arr[n++] = node;
    return n;
}



int exec_pipe(ASTNode *node){
    ASTNode *cmds[128];
    int n = create_pipeline(node, cmds);  // получаем список команд

    int pipefd[2];
    int prev_fd = -1;

    pid_t pgid = 0;
	job_add(&jobs, job_create(full_command, pgid, 1));

    for (int i = 0; i < n; i++) {
        if (i < n - 1) {
            if (pipe(pipefd) < 0) {
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
            // --- CHILD ---
            if (pgid == 0)
                pgid = getpid();          // первая команда становится лидером
            setpgid(0, pgid);

            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

            if (prev_fd != -1) {
                dup2(prev_fd, STDIN_FILENO);
            }

            if (i < n - 1) {
                dup2(pipefd[1], STDOUT_FILENO);
            }

            if (prev_fd != -1) close(prev_fd);
            if (i < n - 1) {
                close(pipefd[0]);
                close(pipefd[1]);
            }

            execvp(cmds[i]->argv[0], cmds[i]->argv);
            perror("execvp");
            exit(1);
        }

        // --- PARENT ---
        if (pgid == 0) pgid = pid;
        setpgid(pid, pgid);

        if (prev_fd != -1)
            close(prev_fd);

        if (i < n - 1) {
            close(pipefd[1]);
            prev_fd = pipefd[0];
        }
    }


    tcsetpgrp(STDIN_FILENO, pgid);

    // --- ЖДЁМ ВСЮ ГРУППУ ---
    while (1) {
        int status;
        pid_t w = waitpid(-pgid, &status, WUNTRACED);

        if (w == -1) break;

        if (WIFSTOPPED(status)) {
            tcsetpgrp(STDIN_FILENO, getpid());
            return 0;
        }

        if (WIFEXITED(status) || WIFSIGNALED(status)) {
        }
    }

    tcsetpgrp(STDIN_FILENO, getpid());
    return 0;
}

int exec_and_or(ASTNode *node){
    int status = execute(node->left);

    if (node->op == AND_IF) {     // &&
        if (status == 0)          // success
            return execute(node->right);
        else
            return status;        // skip right
    }

    if (node->op == OR_IF) {      // ||
        if (status != 0)          // fail
            return execute(node->right);
        else
            return status;        // skip right
    }

    return status;
}

int exec_seq(ASTNode *node){
    if(node->op == SEPARATOR){
        execute(node->left);
        return execute(node->right);
    }

    if(node->op == BACKGROUND){
        node->left->background = 1;
        execute(node->left);
        return execute(node->right);
    }
    return 0;
}



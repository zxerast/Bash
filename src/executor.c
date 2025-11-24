#include "executor.h"

int execute(ASTNode *node, int stage) {
    if (!node) return 0;

    switch (node->type) {

        case NODE_COMMAND:
            return exec_command(node, stage);

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
                    perror("open 2>"); 
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


int exec_command(ASTNode *node, int is_fork){
	
    if(is_fork == 1){
        if (redirects(node->redirects) < 0){
            exit(1);	
        }
    
        execvp(node->argv[0], node->argv);
		perror("execvp");
		exit(1);
    }

    pid_t pid = fork();
	
	if(pid < 0){
		perror ("fork");
		return 1;
	} 
	
    if(pid == 0){
        if (redirects(node->redirects) < 0){
            exit(1);	
        }

        execvp(node->argv[0], node->argv);
		perror("execvp");
		exit(1);
	}	

    int status;
    waitpid(pid, &status, 0);

    return exitstatus(status);
}



int exec_pipe(ASTNode *node){
    int pipefd[2];
   
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t left_pid = fork();

    if (left_pid < 0) {
        perror("fork");
        return 1;
    }

    if (left_pid == 0) {
        dup2(pipefd[1], STDOUT_FILENO);

        if (node->op == PIPE_AND) {
            dup2(pipefd[1], STDERR_FILENO);
        }

        close(pipefd[0]);
        close(pipefd[1]);

        int status = execute(node->left, 1);
        exit(status);
    }

    pid_t right_pid = fork();
    
    if (right_pid < 0) {
        perror("fork");
        return 1;
    }

    if (right_pid == 0) {
        dup2(pipefd[0], STDIN_FILENO);

        close(pipefd[0]);
        close(pipefd[1]);

        int status = execute(node->right, 1);
        exit(status);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    int status_left, status_right;
    waitpid(left_pid, &status_left, 0);
    waitpid(right_pid, &status_right, 0);

    return exitstatus(status_right);
}

int exec_and_or(ASTNode *node){
    int status = execute(node->left, 0);

    if (node->op == AND_IF) {     // &&
        if (status == 0)          // success
            return execute(node->right, 0);
        else
            return status;        // skip right
    }

    if (node->op == OR_IF) {      // ||
        if (status != 0)          // fail
            return execute(node->right, 0);
        else
            return status;        // skip right
    }

    return status;
}

int exec_seq(ASTNode *node){
    if(node->op == SEPARATOR){
        execute(node->left, 0);
        return execute(node->right, 0);
    }

    if(node->op == BACKGROUND){
        pid_t pid = fork();

        if(pid < 0) {
            perror("fork");
            return 1;
        }
        
        if(pid == 0){
            exit(execute(node->left, 1));
        }
        return execute(node->right, 0);
    }
    return 0;
}

int exitstatus(int status){
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    else
        return 1;
}


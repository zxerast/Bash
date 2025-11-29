#include "builtins.h"
#include "ast.h"
#include "executor.h"

#define HISTORY_FILE ".my_shell_history"

int builtin(ASTNode *node){
    if (redirects(node->redirects) < 0){
        exit(1);	
    }
    if(strcmp(node->argv[0], "cd") == 0){
        return builtin_cd(node->argv);
    }
    else if(strcmp(node->argv[0], "exit") == 0){
        return builtin_exit(node->argv);
    }
    else if(strcmp(node->argv[0], "pwd") == 0){
        return builtin_pwd();
    }
    else if(strcmp(node->argv[0], "echo") == 0){
        return builtin_echo(node->argv);
    }
    else if(strcmp(node->argv[0], "help") == 0){
        return builtin_help();
    }
    else if(strcmp(node->argv[0], "history") == 0){
        return builtin_history();
    }
    return 1;
}

int builtin_cd(char **argv){
    if(argv[1] == NULL){
        chdir(getenv("HOME"));
        return 0;
    }
    if(chdir(argv[1]) != 0){
        perror("cd");
        return 1;
    }
    return 0;
}

int builtin_exit(char **argv){
    int status = 0;
    if(argv[1] != NULL){
        status = atoi(argv[1]);
    }
    exit(status);
    return 0; // не будет выполнено
}

int builtin_pwd(){
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0;
    } 
    else {
        perror("pwd");
        return 1;
    }
}

int builtin_echo(char **argv){
    for(int i = 1; argv[i] != NULL; i++){
        printf("%s", argv[i]);
        if(argv[i + 1] != NULL){
            printf(" ");
        }
    }
    printf("\n");
    return 0;
}
int builtin_help(){
    printf("Supported built-in commands:\n");
    printf("cd [dir]    - Change the current directory to 'dir'. If 'dir' is not provided, changes to HOME.\n");
    printf("exit [n]    - Exit the shell with status 'n'. If 'n' is not provided, exits with status 0.\n");
    printf("pwd         - Print the current working directory.\n");
    printf("echo [args] - Print 'args' to the standard output.\n");
    printf("help        - Display this help message.\n");
    return 0;
}

int builtin_history(){
    FILE *file = fopen(HISTORY_FILE, "r");
    if (file == NULL) {
        perror("history");
        return 1;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        printf("%s", line);
    }

    fclose(file);
    return 0;
}
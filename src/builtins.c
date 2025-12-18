#define _GNU_SOURCE

#include "builtins.h"
#include "ast.h"
#include "executor.h"
#include "job_control.h"
#include <ctype.h>
#include "variables.h"

#define HISTORY_FILE ".my_shell_history"    // файл истории

extern JobList jobs;

Builtin is_builtin(ASTNode *node) {
    if (!node || !node->argv || !node->argv[0]) 
        return NONE;

    char *s = node->argv[0];

    if (strcmp(s, "cd") == 0) return CD;
    if (strcmp(s, "exit") == 0) return EXIT;        // проверяем на встроенные команды
    if (strcmp(s, "pwd") == 0) return PWD;
    if (strcmp(s, "echo") == 0) return ECHO;
    if (strcmp(s, "help") == 0) return HELP;
    if (strcmp(s, "history") == 0) return HISTORY;
    if (strcmp(s, "jobs") == 0) return JOBS;
    if (strcmp(s, "fg") == 0) return FG;
    if (strcmp(s, "bg") == 0) return BG;
    if (strcmp(s, "kill") == 0) return KILL;
    if (strcmp(s, "export") == 0) return EXPORT;

    return NONE;
}


int builtin(ASTNode *node){
    Builtin b = is_builtin(node);

    switch (b) {
        case CD: return builtin_cd(node->argv);
        case EXIT: return builtin_exit(node->argv); // исполняем соответствующую
        case PWD: return builtin_pwd();
        case ECHO: return builtin_echo(node->argv);
        case HELP: return builtin_help();
        case HISTORY: return builtin_history();
        case JOBS: return builtin_jobs(&jobs);
        case FG: return builtin_fg(node->argv);
        case BG: return builtin_bg(node->argv);
        case KILL: return builtin_kill(node->argv);
        case EXPORT: return builtin_export(node->argv);
        default: return 1;
    }
}

int builtin_cd(char **argv){
    if(argv[1] == NULL){
        chdir(getenv("HOME"));  // если нет аргумента — в домашнюю директорию
        return 0;
    }
    if(chdir(argv[1]) != 0){    // меняем директорию и проверяем на ошибки
        perror("cd");
        return 1;
    }
    return 0;
}

int builtin_exit(char **argv){
    int status = 0;
    if(argv[1] != NULL){        
        status = atoi(argv[1]); // если есть аргумент — используем его как статус выхода
    }
    exit(status);
    return 0; 
}

int builtin_pwd(){
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) { // получаем текущую директорию
        printf("%s\n", cwd);    // и выводим её
        return 0;
    } 
    else {
        perror("pwd");
        return 1;
    }
}

int builtin_echo(char **argv){
    for(int i = 1; argv[i] != NULL; i++){   
        printf("%s", argv[i]);  //  выводим все аргументы через пробел
        if(argv[i + 1] != NULL){ 
            printf(" "); 
        }
    }
    printf("\n");
    return 0;
}


int builtin_help(){ // просто выводим текст
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
    while (fgets(line, sizeof(line), file)) {   // читаем построчно историю и выводим
        printf("%s", line);
    }

    fclose(file);
    return 0;
}

int builtin_jobs(JobList *jobs){
    Job *j = jobs->head;
    while (j != NULL) {
        char *stat;
        if (j->stopped == 1){
            stat = "Stopped";
        }

        else if (j->running == 1){
            stat = "Running";
        }

        else if (j->exited == 1){
            stat = "Finished";
        }
        
        printf("[%d] %s %s\n", j->id, j->cmd, stat); // пока список не пуст — выводим все job'ы
        j = j->next;
    }
    return 0;
}

int builtin_bg(char **argv) {
    
    if(argv[1] == NULL){
        fprintf(stderr, "bg: missing job id\n");
        return 1;
    }
    int id = atoi(argv[1]);

    Job *j = find_job(&jobs, id);
    if (!j) {
        fprintf(stderr, "bg: job %d not found\n", id);
        return 1;
    }

    j->background = 1;

    if (kill(-j->pgid, SIGCONT) < 0) {  // посылаем сигнал продолжения
        perror("bg: SIGCONT");
        return 1;
    }

    printf("[%d] %s &\n", j->id, j->cmd);   // выводим сообщение о возобновлении в фоне
    return 0;
}

int builtin_fg(char **argv) {
    if(argv[1] == NULL){
        fprintf(stderr, "fg: missing job id\n");
        return 1;
    }
    int id = atoi(argv[1]);

    Job *j = find_job(&jobs, id);
    if (!j) {
        fprintf(stderr, "fg: job %d not found\n", id);
        return 1;
    }

    j->background = 0;

    if (tcsetpgrp(STDIN_FILENO, j->pgid) < 0) { // передача терминала группе процессов
        perror("fg: tcsetpgrp");
        return 1;
    }

    if (j->stopped) {       // если была остановлена — посылаем SIGCONT чтобы запустить
        if (kill(-j->pgid, SIGCONT) < 0) {
            perror("fg: SIGCONT");
            tcsetpgrp(STDIN_FILENO, getpid());
            return 1;
        }
        j->stopped = 0;
    }

    int status;

    while (1) {
        pid_t w = waitpid(-j->pgid, &status, WUNTRACED | WCONTINUED); // ждём

        if (w == -1) break; // не дождались(

        if (WIFSTOPPED(status)) {   // встали
            for (int i = 0; i < j->proc_count; i++) {
                if (j->procs[i].pid == w) {
                    j->procs[i].stopped = 1;
                    break;
                }
            }
        }

        if (WIFCONTINUED(status)) {
            for (int i = 0; i < j->proc_count; i++) {
                if (j->procs[i].pid == w) {
                    j->procs[i].stopped = 0;
                    break;
                }
            }
        }

        if (WIFEXITED(status) || WIFSIGNALED(status)) { // завершились либо сдохли
            for (int i = 0; i < j->proc_count; i++) {
                if (j->procs[i].pid == w) {
                    j->procs[i].finished = 1;
                    break;
                }
            }
        }

        if (all_procs_stopped(j)) {
            j->stopped = 1;
            printf("\n[%d] Stopped         %s", j->id, j->cmd);
            break;
        }

        if (all_procs_finished(j)) {
            j->exited = 1;
            job_remove(&jobs, j);
            break;
        }
    }

    tcsetpgrp(STDIN_FILENO, getpid());
    return 0;
}

int builtin_kill(char **argv) {
    int sig = SIGTERM;     // сигнал по умолчанию
    int arg_i = 1;

    if (argv[1] == NULL) {
        fprintf(stderr, "kill: missing argument\n");
        return 1;
    }

    if (argv[1][0] == '-') {
        char *s = argv[1] + 1;

        if (isdigit(s[0])) {
            sig = atoi(s);
        } 
        
        else {
            if (strncasecmp(s, "SIG", 3) == 0)  // отправка сигнала с префиксом SIG
                s += 3;

            sig = str2sig(s);   // преобразование строки в номер сигнала
            if (sig < 0) {
                fprintf(stderr, "kill: invalid signal %s\n", argv[1]);
                return 1;
            }
        }

        arg_i = 2;
        if (argv[arg_i] == NULL) {
            fprintf(stderr, "kill: missing pid or job\n");
            return 1;
        }
    }

    char *t = argv[arg_i];

    if (t[0] == '%') {
        int jid = atoi(t + 1);
        Job *j = find_job(&jobs, jid);

        if (!j) {
            fprintf(stderr, "kill: no such job %s\n", t);
            return 1;
        }

        if (kill(-j->pgid, sig) < 0) {
            perror("kill");
            return 1;
        }

        return 0;
    }

    int pid = atoi(t);

    if (pid <= 0) {
        fprintf(stderr, "kill: invalid pid %s\n", t);
        return 1;
    }

    if (kill(pid, sig) < 0) {
        perror("kill");
        return 1;
    }

    return 0;
}

int str2sig(const char *name) {     // преобразование строки в номер сигнала
    struct {
        const char *name;
        int sig;
    } table[] = {   // таблица соответствий
        {"HUP",  SIGHUP},
        {"INT",  SIGINT},
        {"QUIT", SIGQUIT},
        {"ILL",  SIGILL},
        {"ABRT", SIGABRT},
        {"FPE",  SIGFPE},
        {"KILL", SIGKILL},
        {"SEGV", SIGSEGV},
        {"PIPE", SIGPIPE},
        {"ALRM", SIGALRM},
        {"TERM", SIGTERM},
        {"STOP", SIGSTOP},
        {"TSTP", SIGTSTP},
        {"CONT", SIGCONT},
        {"CHLD", SIGCHLD},
        {NULL, 0}
    };

    for (int i = 0; table[i].name; i++)
        if (strcasecmp(table[i].name, name) == 0)   // сравнение без учёта регистра
            return table[i].sig;

    return -1;
}


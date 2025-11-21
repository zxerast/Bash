#pragma once

#include <pwd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

char *create_prompt() {
    char hostname[HOST_NAME_MAX + 1];   //  константы из limits.h
    char cwd[PATH_MAX + 1];
    struct passwd *pw;	//	структура хранящая имя пользоваттеля
    char *username;

    pw = getpwuid(getuid());	// проверяем существование пользователя
    if (pw) {
        username = pw->pw_name;
    } 
    else {
        username = "unknown";
    }

    if (gethostname(hostname, sizeof(hostname)) != 0) {
        strcpy(hostname, "unknown");	// имя хоста
    }

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        strcpy(cwd, "?");	// текущая директория
    }
    size_t size = strlen(username) + strlen(hostname) + strlen(cwd) + 4;
    char *prompt = malloc(size);       // username@hostname:cwd$ + '\0'

    sprintf(prompt, "%s@%s:%s$ ", username, hostname, cwd);

    return prompt;
}

#include "variables.h"

char *expand_var(const char *s) {
    if (s[0] != '$') return strdup(s);  //  Если это не переменная окружения просто берём строку

    const char *name = s + 1;   //  Указатель на символы после $
    char *val = getenv(name);   //  Ищем в записях

    if (val == NULL) return strdup("");     // не нашли
    return strdup(val); //  нашли
}

int builtin_export(char **argv) {
    if (argv[1] == NULL) {
        fprintf(stderr, "export: missing argument\n");
        return 1;
    }

    char *arg = argv[1];
    char *ravno = strchr(arg, '='); //   Ищем присвоение

    if (!ravno) {
        fprintf(stderr, "export: use VAR=value\n");
        return 1;
    }

    *ravno = '\0';  //  Разделяем на
    char *name = arg;   // Ключ
    char *value = ravno + 1; // и значение

    if (setenv(name, value, 1) < 0) {   //  Ну и сохраняем
        perror("setenv");
        return 1;
    }

    return 0;
}

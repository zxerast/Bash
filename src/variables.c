#include "variables.h"

char *expand_var(const char *s) {
    if (s[0] != '$') return strdup(s);

    const char *name = s + 1;
    char *val = getenv(name);

    if (val == NULL) return strdup("");  
    return strdup(val);
}

int builtin_export(char **argv) {
    if (argv[1] == NULL) {
        fprintf(stderr, "export: missing argument\n");
        return 1;
    }

    char *arg = argv[1];
    char *ravno = strchr(arg, '=');

    if (!ravno) {
        fprintf(stderr, "export: use VAR=value\n");
        return 1;
    }

    *ravno = '\0';
    char *name = arg;
    char *value = ravno + 1;

    if (setenv(name, value, 1) < 0) {
        perror("setenv");
        return 1;
    }

    return 0;
}

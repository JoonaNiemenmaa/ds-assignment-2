#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char *str;
    int capacity;
} string_t;

string_t string(char *str) {
    char *new = NULL;
    int capacity = (strlen(str) + 1) * sizeof(char);

    if ((new = (char *)malloc(capacity)) == NULL) {
        perror("error");
        exit(1);
    }

    strcpy(new, str);

    return (string_t) {
        .str = new,
        .capacity = capacity
    };
}

void string_push(string_t *str, char *push) {
    int len = strlen(str->str);
    int needed_capacity = strlen(str->str) + strlen(push) + 1;
    if (str->capacity < needed_capacity) {
        if ((str->str = (char *)realloc(str->str, needed_capacity)) == NULL) {
            perror("error");
            exit(1);
        }
        str->capacity = needed_capacity;
    }

    str->str[needed_capacity - 1] = '\0';
    char *p = str->str + len;
    for (int i = 0; i < strlen(push); i++) {
        *p = push[i];
        p++;
    }
}

void string_push_ch(string_t *str, char c) {
    int needed_capacity = strlen(str->str) + 2;
    if (str->capacity < needed_capacity) {
        if ((str->str = (char *)realloc(str->str, needed_capacity)) == NULL) {
            perror("error");
            exit(1);
        }
        str->capacity = needed_capacity;
    }

    str->str[needed_capacity - 2] = c;
    str->str[needed_capacity - 1] = '\0';
}

char string_pop_ch(string_t str) {
    int len = strlen(str.str);
    if (len > 0) {
        char c = str.str[len - 1];
        str.str[len - 1] = '\0';
        return c;
    } else {
        return '\0';
    }
}

void string_clear(string_t str) {
    str.str[0] = '\0';
}

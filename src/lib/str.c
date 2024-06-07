#include "str.h"
#include <stdlib.h>


u64 str_len(const char* str) {
    u64 len = 0;
    while (str[len] != '\0') { len++; }
    return len;
}

i32 str_cmp(const char* str_a, const char* str_b) {
    u64 i = 0;
    while (str_a[i] != '\0' && str_b[i] != '\0') {
        if (str_a[i] != str_b[i]) {
            return str_a[i] - str_b[i];
        }
        i++;
    }

    return str_a[i] - str_b[i];
}

char* str_cpy(char* dst, const char* src) {
    u64 i = 0;
    while (src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    return dst;
}

char* str_ncpy(char* dst, const char* src, u64 length) {
    for (u64 i = 0; i < length; ++i) {
        dst[i] = src[i];
    }
    dst[length] = '\0';
    return dst;
}

char* str_dup(const char* str) {
    u64 len = str_len(str);
    char* new_str = (char*)malloc((len + 1) * sizeof(char));
    if (new_str == NULL) {
        return NULL;
    }
    str_cpy(new_str, str);
    return new_str;
}

char* str_cat(char* dst, const char* src) {
    u64 dst_len = str_len(dst);
    u64 i = 0;
    while (src[i] != '\0') {
        dst[dst_len + i] = src[i];
        i++;
    }
    dst[dst_len + i] = '\0';
    return dst;
}

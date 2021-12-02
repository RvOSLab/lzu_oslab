#include <string.h>
#include <stddef.h>

size_t strlen(const char *str)
{
    size_t i = 0;
    for (; str[i] != '\0'; ++i)
        ;
    return i;
}

uint32_t strcmp(const char *s, const char *t)
{
    while (*s && *t && *s == *t) {
        s++;
        t++;
    }
    return *s - *t;
}

int is_begin_with(const char *str1, const char *str2)
{
    if (!str1 || !str2)
        return -1;
    int len1 = strlen(str1);
    int len2 = strlen(str2);
    if ((len1 < len2) || !len1 || !len2)
        return -1;
    const char *p = str2;
    int i = 0;
    while (*p != '\0') {
        if (*p != str1[i])
            return 0;
        p++;
        i++;
    }
    return 1;
}
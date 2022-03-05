#include <string.h>

size_t strlen(const char *str)
{
    size_t i = 0;
    for (; str[i] != '\0'; ++i)
        ;
    return i;
}

void *memset(void *src, char ch, size_t cnt)
{
    for (size_t i = 0; i < cnt; ++i)
        *((char *)src + i) = ch;
    return src;
}

void *memcpy(void *dest, const void *src, size_t n)
{
    void *ret = dest;
    while (n--) {
        /*
         * ++ 和 (type) 运算符优先级相同，从右往左结合，
         * 因此修改的是指针，而不是转型创建的临时值。
         */
        *((char *)dest++) = *((char *)src++);
    }
    return ret;
}

int64_t strcmp(const char *s, const char *t)
{
    while (*s && *t && *s == *t) {
        s++;
        t++;
    }
    return *s - *t;
}

const char *strchr (const char *str, char c)
{
    while(*str) {
        if (*str == c) return str;
        str++;
    }
    return NULL;
}

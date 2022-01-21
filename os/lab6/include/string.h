#ifndef __STRING_H__
#define __STRING_H__
#include <stddef.h>
size_t strlen(const char *str);
void *memset(void *src, char ch, size_t cnt);
void *memcpy(void *dest, const void *src, size_t n);
int64_t strcmp(const char *s, const char *t);
const char *strchr(const char *str, char c);
#endif

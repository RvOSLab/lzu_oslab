#ifndef __STRING_H__
#define __STRING_H__
#include <stddef.h>
size_t strlen(const char *str);
void *memset(void *src, char ch, size_t cnt);
void *memcpy(void *dest, const void *src, size_t n);
#endif

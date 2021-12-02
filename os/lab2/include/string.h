#ifndef __STRING_H__
#define __STRING_H__
#include <stddef.h>
size_t strlen(const char *str);
uint32_t strcmp(const char *s, const char *t);
int is_begin_with(const char *str1, const char *str2);
static inline uint32_t fdt32_to_cpu(uint32_t val)
{
    uint8_t *byte_i = (uint8_t *)&val;
    return (byte_i[0] << 24) | (byte_i[1] << 16) | (byte_i[2] << 8) | byte_i[3];
}

#endif

#ifndef KDEBUG_H
#define KDEBUG_H
#include <stdarg.h>
#include <stddef.h>
#include <sbi.h>
int kputs(const char *msg);
uint64_t kprintf(const char* _Format, ...);
void kputchar(int ch);
void do_panic(const char* file, int line, const char* fmt, ...);
#endif /* end of include guard: KDEBUG_H */
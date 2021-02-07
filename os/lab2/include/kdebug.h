#ifndef KDEBUG_H
#define KDEBUG_H
#include <stdarg.h>
#include <stddef.h>
#include <sbi.h>
int kputs(const char *msg);
int kprintf(const char *fmt, ...);
void kputchar(int ch);
#endif /* end of include guard: KDEBUG_H */

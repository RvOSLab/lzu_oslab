#ifndef __KDEBUG_H__
#define __KDEBUG_H__
#include <stdarg.h>
#include <stddef.h>
#include <sbi.h>
int kputs(const char *msg);
int kprintf(const char *fmt, ...);
void kputchar(char ch);
void do_panic(const char* file, int line, const char* fmt, ...);
#endif

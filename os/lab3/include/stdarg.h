/**
 * @file stdarg.h
 * 本文件实现可变参数
 *
 * @note 可变参数需要编译器的支持，不可能由程序员独立实现。
 *       因此我们只是简单的包装编译器提供的内建函数、类型。
 */
#ifndef __STDARG_H__
#define __STDARG_H__
typedef __builtin_va_list va_list;
#define va_start(ap, last) (__builtin_va_start(ap, last))
#define va_arg(ap, type) (__builtin_va_arg(ap, type))
#define va_end(ap)
#endif

/**
 * @file syscall.h
 * @brief 声明系统调用号、系统调用表和系统调用创建宏
 */
#ifndef __SYSCALL_H__
#define __SYSCALL_H__
#include <trap.h>
typedef long (*fn_ptr)(struct trapframe *);                 /**< 系统调用指针类型 */
extern fn_ptr syscall_table[];
extern long test_fork;
/// @{ @name 系统调用号
#define NR_syscalls 5                                      /**< 系统调用数量 */
#define NR_fork     1
#define NR_test_fork 2
#define NR_getpid 3
#define NR_getppid 4
/// @}

long syscall(long number, ...);

#endif /* end of include guard: __SYSCALL_H__ */

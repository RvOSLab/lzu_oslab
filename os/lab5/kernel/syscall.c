/**
 * @file syscall.c
 * @brief 定义系统调用表，实现大部分系统调用
 */
#include <trap.h>
#include <kdebug.h>
#include <syscall.h>

extern int sys_init(struct trapframe *);

/**
 * @brief 系统调用表
 * 存储所有系统调用的指针的数组，系统调用号是其中的下标。
 * 所有系统调用都通过系统调用表调用
 */
fn_ptr syscall_table[] = {sys_init};

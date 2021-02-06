#ifndef __SYSCALL_H__
#define __SYSCALL_H__
#include <trap.h>
typedef int (*fn_ptr)(struct trapframe *);                 /**< 系统调用指针类型 */
extern fn_ptr syscall_table[];
/// @{ @name 系统调用号
/// 系统调用号宏格式为 NR_name
#define NR_syscalls 2                                      /**< 系统调用数量 */
#define NR_fork
#define NR_testsyscall 0

/// @}
/// @{ @name 创建系统调用
/// @note
/// - 暂时不能保证系统调用可以正确传递参数，获取返回值。
/// - 大多系统调用成功返回 0,失败返回 -1,但存在例外，如 `getpriority()`
///   返回进程（组）优先级，可能是负数，这里没有考虑这种情况。
/// @todo 正确处理 note 中的情况

/**
 * @brief 创建无参数的系统调用
 * @note 这个函数暂时基本可以使用，虽然返回值可能有错
 *       参数大概率可以正确传递
 */
#define _syscall0(type, name)                               \
	type name(void)                                         \
	{                                                       \
		register long __res asm("a0");                      \
		register long a7 asm("a7") = NR_##name;             \
		a7 = NR_##name;                                     \
		__asm__ volatile("ecall \n\t"                       \
				 : "=r"(__res)                              \
				 : "r"(a7)                                  \
				 : "memory");                               \
		if (__res >= 0)                                     \
			return (type)__res;                             \
		return -1;                                          \
	}

/**
 * @brief创建只有一个参数的系统调用
 * @note 这是 Linux0.12 的代码，不能使用
 */
#define _syscall1(type, name, atype, a)                     \
	type name(atype a)                                      \
	{                                                       \
		long __res;                                         \
		__asm__ volatile("int $0x80"                        \
				 : "=a"(__res)                              \
				 : "0"(__NR_##name), "b"((long)(a)));       \
		if (__res >= 0)                                     \
			return (type)__res;                             \
		errno = -__res;                                     \
		return -1;                                          \
	}

/**
 * @brief创建只有两个参数的系统调用
 * @note 这是 Linux0.12 的代码，不能使用
 */
#define _syscall2(type, name, atype, a, btype, b)           \
	type name(atype a, btype b)                             \
	{                                                       \
		long __res;                                         \
		__asm__ volatile("int $0x80"                        \
				 : "=a"(__res)                              \
				 : "0"(__NR_##name), "b"((long)(a)),        \
				   "c"((long)(b)));                         \
		if (__res >= 0)                                     \
			return (type)__res;                             \
		errno = -__res;                                     \
		return -1;                                          \
	}

/**
 * @brief创建只有三个参数的系统调用
 * @note 这是 Linux0.12 的代码，不能使用
 */
#define _syscall3(type, name, atype, a, btype, b, ctype, c) \
	type name(atype a, btype b, ctype c)                    \
	{                                                       \
		long __res;                                         \
		__asm__ volatile("int $0x80"                        \
				 : "=a"(__res)                              \
				 : "0"(__NR_##name), "b"((long)(a)),        \
				   "c"((long)(b)), "d"((long)(c)));         \
		if (__res >= 0)                                     \
			return (type)__res;                             \
		errno = -__res;                                     \
		return -1;                                          \
	}
/// @}

/**
 * @file syscall.h
 * @brief 声明系统调用号、系统调用表和系统调用创建宏
 */
#endif /* end of include guard: __SYSCALL_H__ */

/**
 * 本文件定义常用数据类型
 */
#ifndef __STDDEF_H__
#define __STDDEF_H__

#define NULL (void *)0
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned long long size_t;
typedef signed long long ssize_t;
/** 
 * 不要使用 intptr_t, uintptr_t，请用 int64_t, uint64_t 代替
 * typedef int64_t intptr_t;
 * typedef uint64_t uintptr_t;
 */

#endif

/**
 * @file clock.h
 * @author Hanabichan (93yutf@gmail.com)
 * @brief 本文件声明时钟中断相关的全局变量和函数
 */
#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <stddef.h>
#include <sbi.h>

extern volatile size_t ticks;

void clock_init();
void clock_set_next_event();

#endif

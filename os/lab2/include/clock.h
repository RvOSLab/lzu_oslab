/**
 * @file clock.h
 * @author Hanabichan (93yutf@gmail.com)
 * @brief 本文件声明时钟中断中的外部变量 ticks 与函数 clock_init() 和 clock_set_next_event()
 */
#ifndef __CLOCK_H__
#define __CLOCK_H__

/** 导入库 */
#include <stddef.h>
#include <sbi.h>

/** 外部变量ticks，定义在 clock.c 中 */
extern volatile size_t ticks;

void clock_init();
void clock_set_next_event();

#endif

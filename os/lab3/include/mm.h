/**
 * @file mm.h
 * @brief 声明内存管理模块的宏、函数、全局变量
 */
#ifndef __MM_H__
#define __MM_H__
#include <stddef.h>
/// @{ @name 物理内存布局和物理地址操作
#define PAGE_SIZE 4096
#define FLOOR(addr) ((addr) / PAGE_SIZE * PAGE_SIZE)/**< 向下取整到 4K 边界 */
#define CEIL(addr)               \
    (((addr) / PAGE_SIZE + ((addr) % PAGE_SIZE != 0)) * PAGE_SIZE) /**< 向上取整到 4K 边界 */
#define DEVICE_START    0x10000000                  /**< 设备树地址空间，暂时不使用 */
#define DEVICE_END      0x10010000
#define MEM_START       0x80000000                  /**< 物理内存地址空间 */
#define MEM_END         0x88000000
#define SBI_START       0x80000000                  /**< SBI 物理内存起始地址 */
#define SBI_END         0x80200000                  /**< 用户程序（包括内核）可用的物理内存地址空间开始 */
#define HIGH_MEM        0x88000000                  /**< 空闲内存区结束 */
#define LOW_MEM         0x82000000                  /**< 空闲内存区开始（可用于用户进程和数据放置） */
#define PAGING_MEMORY   (1024 * 1024 * 128)         /**< 系统物理内存大小 (bytes) */
#define PAGING_PAGES    (PAGING_MEMORY >> 12)       /**< 系统物理内存页数 */
#define MAP_NR(addr)    (((addr)-MEM_START) >> 12)  /**< 物理地址 addr 在 mem_map[] 中的下标 */
/// @}

/// @{ @name 物理页标志位
#define USED 100
#define UNUSED 0
/// @}

extern unsigned char mem_map[PAGING_PAGES];         /**< 物理内存位图,记录物理内存使用情况 */
void mem_test();
void mem_init();
void free_page(uint64_t addr);
uint64_t get_free_page(void);

#endif

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
#define PAGING_MEMORY   (1024 * 1024 * 128)         /**< 系统物理内存大小 */
#define PAGING_PAGES    (PAGING_MEMORY >> 12)       /**< 系统物理内存页数 */
#define MAP_NR(addr)    (((addr)-MEM_START) >> 12)  /**< 物理地址 addr 在 mem_map[] 中的下标 */
/// @}

/// @{ @name 虚拟
/* BASE_ADDRESS   -- 0xC0000000 */
/* DEVICE_ADDRESS -- 0xC8000000 */
#define KERNEL_ADDRESS    (MEM_START + LINEAR_OFFSET)
#define DEVICE_ADDRESS  (KERNEL_ADDRESS + PAGING_MEMORY)
/// @}

/// @{ @name 物理页标志位
#define USED 100
#define UNUSED 0
/// @}

/// @{ @name 页表项标志位
#define PAGE_DIRTY        0x80
#define PAGE_ACCESSED    0x40
#define PAGE_USER        0x10
#define PAGE_READABLE   0x02
#define PAGE_WRITABLE   0x04
#define PAGE_EXECUTABLE 0x08
#define PAGE_PRESENT    0x01

#define KERN_RWX       (PAGE_READABLE   | PAGE_WRITABLE | PAGE_EXECUTABLE)
#define KERN_RW        (PAGE_READABLE   | PAGE_WRITABLE)
#define USER_RWX       (PAGE_USER       | PAGE_READABLE | PAGE_WRITABLE | PAGE_EXECUTABLE)
#define USER_RX        (PAGE_USER       | PAGE_READABLE | PAGE_EXECUTABLE)
#define USER_RW        (PAGE_USER       | PAGE_READABLE | PAGE_WRITABLE)
#define USER_R         (PAGE_USER       | PAGE_READABLE)
/// @}

/** 刷新 TLB */
#define invalidate() __asm__ __volatile__("sfence.vma\n\t"::)

/// @{ @name 虚拟地址操作
#define GET_VPN1(addr)     (( (addr) >> 30) & 0x1FF)
#define GET_VPN2(addr)     (( (addr) >> 21) & 0x1FF)
#define GET_VPN3(addr)     (( (addr) >> 12) & 0x1FF)
#define GET_PPN(addr)      (( addr) >> 12)
#define GET_PAGE_ADDR(pte) (( (pte) & ~0xFFC00000000003FF) << 2)
#define GET_FLAG(pte)      ( (pte) & 0x3FF )
#define LINEAR_OFFSET   0x40000000
#define PHYSICAL(addr)  (addr - LINEAR_OFFSET)
#define VIRTUAL(addr)   (addr + LINEAR_OFFSET)
/* 必须保证 end > start */
#define IS_KERNEL(start, end) (start >= KERNEL_ADDRESS && end <= KERNEL_ADDRESS + PAGING_MEMORY)
#define IS_USER(start, end)   (end <= KERNEL_ADDRESS)
/// @}

extern unsigned char mem_map [ PAGING_PAGES ];
extern uint64_t *pg_dir;

/// @{ @name 内核地址
/// 可执行文件中各节的起始虚拟地址,定义在链接脚本中
extern void kernel_start();
extern void kernel_end();
extern void text_start();
extern void rodata_start();
extern void data_start();
extern void bss_start();
/// @}

void mem_test();
void mem_init();
void free_page(uint64_t addr);
void free_page_tables(uint64_t from, uint64_t size);
int copy_page_tables(uint64_t from, uint64_t *to_pg_dir, uint64_t to, uint64_t size);
uint64_t get_free_page(void);
void write_verify(uint64_t addr);
void get_empty_page(uint64_t addr, uint8_t flag);
uint64_t put_page(uint64_t page, uint64_t addr, uint8_t flag);
void show_page_tables();
void map_kernel();
void active_mapping();
// void wp_page_handler(struct trapframe *frame) /* 在完成进程后实现 */

/**
 * @file mm.h
 * 在阅读代码时要分清物理地址和虚拟地址，否则会导致混乱。
 * 本模块注释中专门写了函数参数是物理地址还是虚拟地址，如果没有写，默认是虚拟地址。
 */
#endif

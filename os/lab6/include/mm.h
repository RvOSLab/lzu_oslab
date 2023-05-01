/**
 * @file mm.h
 * @brief 声明内存管理模块的宏、函数、全局变量
 *
 * 在阅读代码时要分清物理地址和虚拟地址，否则会导致混乱。
 * 本模块注释中专门写了函数参数是物理地址还是虚拟地址，如果没有写，默认是虚拟地址。
 */
#ifndef __MM_H__
#define __MM_H__
#include <asm/page.h>
#include <compiler.h>
#include <device/fdt.h>
#include <pgtable.h>
#include <riscv.h>
#include <stddef.h>
#include <mmzone.h>

/// @{ @name 物理内存布局和物理地址操作
#define DEVICE_START 0x10000000 /**< 设备树地址空间，暂时不使用 */
#define DEVICE_END 0x10010000
#define MEM_START 0x80000000 /**< 物理内存地址空间 */
#define MEM_END 0x88000000
#define SBI_START 0x80000000 /**< SBI 物理内存起始地址 */
#define SBI_END                                                                \
    0x80200000 /**< 用户程序（包括内核）可用的物理内存地址空间开始 */
#define HIGH_MEM 0x88000000 /**< 空闲内存区结束 */
#define LOW_MEM 0x82000000 /**< 空闲内存区开始（可用于用户进程和数据放置） */
#define PAGING_MEMORY (1024 * 1024 * 128) /**< 系统物理内存大小 (bytes) */
#define PAGING_PAGES (PAGING_MEMORY >> 12) /**< 系统物理内存页数 */
#define MAP_NR(addr)                                                           \
    (((addr)-MEM_START) >> 12) /**< 物理地址 addr 在 mem_map[] 中的下标 */
/// @}

/// @{ @name 虚拟
/* BASE_ADDRESS   -- 0xC0000000 */
/* DEVICE_ADDRESS -- 0xC8000000 */
#define KERNEL_ADDRESS (MEM_START + LINEAR_OFFSET)
#define DEVICE_ADDRESS (KERNEL_ADDRESS + PAGING_MEMORY)
/// @}

/// @{ @name 物理页标志位
#define USED 100
#define UNUSED 0
/// @}

/// @{ @name 页表项标志位
#define PAGE_DIRTY 0x80
#define PAGE_ACCESSED 0x40
#define PAGE_USER 0x10
#define PAGE_READABLE 0x02
#define PAGE_WRITABLE 0x04
#define PAGE_EXECUTABLE 0x08
#define PAGE_VALID 0x01

#define KERN_RWX (PAGE_READABLE | PAGE_WRITABLE | PAGE_EXECUTABLE)
#define KERN_RW (PAGE_READABLE | PAGE_WRITABLE)
#define KERN_RX (PAGE_READABLE | PAGE_EXECUTABLE)
#define USER_RWX (PAGE_USER | PAGE_READABLE | PAGE_WRITABLE | PAGE_EXECUTABLE)
#define USER_RX (PAGE_USER | PAGE_READABLE | PAGE_EXECUTABLE)
#define USER_RW (PAGE_USER | PAGE_READABLE | PAGE_WRITABLE)
#define USER_R (PAGE_USER | PAGE_READABLE)
/// @}

/** 刷新 TLB */
#define invalidate() __asm__ __volatile__("sfence.vma\n\t" ::)

/// @{ @name 虚拟地址操作
#define GET_VPN1(addr) (((addr) >> 30) & 0x1FF)
#define GET_VPN2(addr) (((addr) >> 21) & 0x1FF)
#define GET_VPN3(addr) (((addr) >> 12) & 0x1FF)
#define GET_PPN(addr) ((addr) >> 12)
#define GET_PAGE_ADDR(pte) (((pte)&0x3FFFFFFFFFFC00) << 2)
#define GET_FLAG(pte) ((pte)&0x3FF)
#define LINEAR_OFFSET 0x40000000
#define PHYSICAL(vaddr) (vaddr - LINEAR_OFFSET)
#define VIRTUAL(paddr) (paddr + LINEAR_OFFSET)
/* 必须保证 end > start */
#define IS_KERNEL(start, end)                                                  \
    (start >= KERNEL_ADDRESS && end <= KERNEL_ADDRESS + PAGING_MEMORY)
#define IS_USER(start, end) (end <= KERNEL_ADDRESS)
/// @}

extern unsigned char mem_map[PAGING_PAGES];
extern uint64_t *pg_dir;

// 内核镜像各段起始虚拟地址,定义在链接脚本中
extern char __text;
extern char __text_end;
extern char __rodata;
extern char __rodata_end;
extern char __data;
extern char __data_end;
extern char __bss;
extern char __bss_end;
extern char __end;

void mem_test();
void mem_init();
void free_page(uint64_t addr);
void free_page_tables(uint64_t from, uint64_t size);
int copy_page_tables(uint64_t from, uint64_t *to_pg_dir, uint64_t to,
                     uint64_t size);
void write_verify(uint64_t addr);
void get_empty_page(uint64_t addr, uint16_t flag);
uint64_t put_page(uint64_t page, uint64_t addr, uint16_t flag);
void show_page_tables();
void map_kernel();
void active_mapping();

void probe_mem_areas(const struct fdt_header *fdt);
void mem_init();
void clear_bss();

#endif

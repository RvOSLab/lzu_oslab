#ifndef __MM_H__
#define __MM_H__
#include <stddef.h>
#define PAGE_SIZE       4096
#define FLOOR(addr)     ((addr) / PAGE_SIZE * PAGE_SIZE)
#define CEIL(addr)      (((addr) / PAGE_SIZE + ((addr) % PAGE_SIZE != 0)) * PAGE_SIZE)
#define DEVICE_START    0x10000000
#define DEVICE_END      0x10010000
#define MEM_START       0x80000000
#define MEM_END         0x88000000
#define SIB_START       0x80000000
#define SBI_END         0x82000000
#define HIGH_MEM        0x88000000
#define LOW_MEM         0x83000000
#define PAGING_MEMORY   (1024 * 1024 * 128)
#define PAGING_PAGES    (PAGING_MEMORY>>12)
#define MAP_NR(addr)    (((addr)-MEM_START)>>12)

#define USED 100
#define UNUSED 0

extern unsigned char mem_map [ PAGING_PAGES ];
extern uint64_t pg_dir[512];
void mem_test();
void mem_init();
void free_page(uint64_t addr);
uint64_t get_free_page(void);

#endif

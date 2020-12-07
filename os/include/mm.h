#ifndef MM_H
#define MM_H

#define MEM_START       0x80000000
#define MEM_END         0x88000000
#define HIGH_MEMORY     0x88000000
#define LOW_MEM         0x83000000
#define PAGING_MEMORY   (1024 * 1024 * 128)
#define PAGING_PAGES    (PAGING_MEMORY>>12)
#define MAP_NR(addr)    (((addr)-LOW_MEM)>>12)

#define USED 100

//      记得修改
#define PAGE_DIRTY	    0x40
#define PAGE_ACCESSED	0x20
#define PAGE_USER	    0x04
#define PAGE_RW		    0x02
#define PAGE_PRESENT	0x01

extern unsigned char mem_map [ PAGING_PAGES ];

void mem_test();
void mem_init(long start_mem, long end_mem);
void free_page(uint64_t addr);
uint64_t get_free_page(void);

#endif

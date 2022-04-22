#ifndef __SWAP_H__
#define __SWAP_H__

#include <stddef.h>

#define SWAP_SIZE (2 * 1024 * 1024)   // 2 M
#define SWAP_PAGES (SWAP_SIZE / PAGE_SIZE)

struct swap_map_struct  // 记录某一块内存被换出的情况
{
    uint64_t count; // 引用计数，如果被换出后 fork，引用计数会加 1
    uint64_t swapped_in_paddr;  // 若曾被换入，换入时的新物理地址，若为 0 则表示从未被换入。用于被换出后 fork，第二个进程访问时直接绑定物理地址而不获取新物理页
};

extern struct swap_map_struct swap_map[SWAP_PAGES];

void swap_init();
void swap_in(uint64_t vaddr);
void do_swap_out();

#endif

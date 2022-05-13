#ifndef __SWAP_H__
#define __SWAP_H__

#include <stddef.h>

#define SWAP_SIZE (2 * 1024 * 1024)   // 2 M
#define SWAP_PAGES (SWAP_SIZE / PAGE_SIZE)

extern unsigned char swap_map[SWAP_PAGES];

void swap_init();
void swap_in(uint64_t vaddr);
void do_swap_out();

#endif

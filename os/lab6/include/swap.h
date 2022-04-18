#include <stddef.h>

#define SWAP_SIZE (2 * 1024 * 1024)   // 2 M
#define SWAP_PAGES (SWAP_SIZE / PAGE_SIZE)

void swap_init();
void swap_in(uint64_t *pte);
void swap_out(uint64_t *pte);

#ifndef __PAGE_ALLOC_H__
#define __PAGE_ALLOC_H__
#include <stddef.h>
#include <mmzone.h>

// buddy system allocator options
//
// gfp means 'get free page'
typedef uint32_t gfp_t;
#define GFP_ZONE_MASK 0x03U // Low 2 bits are used to store zone type
#define GFP_WAIT 0x04U // Wthether allow process to sleep
#define GFP_ALLOWD_FLAGS (__GFP_WAIT)
#define GFP_DMA 0x01U
#define GFP_NORMAL 0x00U
#define GFP_ATOMIC GFP_NORMAL // TODO(kongjun18): implement GFP_ATOMIC
#define GFP_KERNEL (GFP_WAIT | GFP_NORMAL)

struct page *alloc_pages(uint32_t order, gfp_t flags);
struct page *alloc_page(gfp_t flags);
void *get_free_pages(uint32_t order);
void *get_free_page();
void free_pages(struct page *page, uint32_t order);

#endif /* __PAGE_ALLOC_H__ */

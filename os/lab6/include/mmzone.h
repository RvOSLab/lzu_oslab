#ifndef __MMZONE__
#define __MMZONE__

#include <stddef.h>
#include <assert.h>
#include <limits.h>
#include <processor.h>
#include <utils/linked_list.h>

#define MAX_NR_NODES 1

#define MAX_GFP_ORDER 10

enum zone_type { ZONE_DMA, ZONE_NORMAL, MAX_NR_ZONES };

// Boot memory allocator used before buddy system.
struct bootmem {
    uint64_t start_pfn; // start pfn of managed RAM
    uint64_t end_pfn; // end pfn of managed RAM

    // a bitmap tracks whether a page is free or not.
    // 0 means free, 1 means used.
    uint8_t *mem_map;

    // used to merge allocate request and implement small aign.
    uint64_t last_pos;
    uint64_t last_offset;
    uint64_t last_success;
};

// Represent a contiguous memory block in buddy system.
struct free_area {
    struct linked_list_node free_list;
    int nr_free;
};

#define PG_RESERVED 0x4U
#define PG_SLAB 0x8U
#define PG_LRU 0x10U
#define PG_PRIVATE 0x20U

// Track the physical page status.
struct page {
    // The `NODE_ZONE_SHIFT` most left bits are used to store node and zone id.
    // Other bits store `gfp_t` flags.
    uint32_t flags;
    // Track how many user used this page.
    // Deallocate page if its count smaller than 1.
    int32_t count;
    // Used to store private data. Only use it if `PG_PRIVATE` flag is set.
    // If the page is free(managed by buddy system) and is the frist page of
    // free area, `private` is the order of the free area.
    uint64_t private;
    // `lru` is a list implementing LRU algorithm.
    // If the page is used in slab allocator, `lru` stores the pointer to
    // slab and cache.
    struct linked_list_node lru;
};

struct zone {
    struct page *mem_map;
    uint64_t start_pfn;
    uint64_t present_pages;
    uint64_t spanned_pages;

    struct free_area free_areas[MAX_GFP_ORDER + 1];
    uint64_t free_pages;
    uint64_t pages_low, pages_min, page_high; // Used in page reclaim

    struct node *node;
};

// Used for zone fallback.
struct zone_list {
    struct zone *zones[MAX_NR_NODES * MAX_NR_ZONES + 1];
};

struct node {
    struct bootmem *bootmem;
    uint64_t start_pfn;
    uint64_t present_pages;
    uint64_t spanned_pages;
    struct page *mem_map;
    struct zone zones[MAX_NR_ZONES];
    struct zone_list zone_lists[MAX_NR_ZONES];

    uint32_t node_id;
};

// Currently only support one node. Thus always return init_node.
extern struct node init_node;
extern struct node *init_nodes[];
#define NODE(nid) init_nodes[0]
#define pfn_to_node(pfn) (&init_node)
#define pfn_to_page(pfn)                                                       \
    ({                                                                         \
        struct node *__node = pfn_to_node((pfn));                              \
        &__node->mem_map[(pfn)-__node->start_pfn];                             \
    })
#define va_to_pfn(addr) (pa((addr)) >> PAGE_SHIFT)
#define va_to_page(addr) pfn_to_page(va_to_pfn(addr))
#define pfn_to_va(pfn) (va((pfn) << PAGE_SHIFT))
#define page_to_va(page) (pfn_to_va(page_pfn((page))))

// NODE_ZONE_SHIFT bits in (struct page).flags are used to store node and zone id.
// +----------+-----+---------------------------+
// |   node   | zone|         page flags        |
// +----------+-----+---------------------------+
//    4 bits   2 bit           26 bits
#define MAX_NODE_SHIFT 4U
#define MAX_ZONE_SHIFT 2U
#define NODE_ZONE_SHIFT                                                        \
    ((sizeof(((struct page *)0)->flags) * 8) - MAX_NODE_SHIFT - MAX_ZONE_SHIFT)
#define NODEZONE(node_id, zone_id) ((node_id << MAX_ZONE_SHIFT) | zone_id)

static inline void clear_page_nodezone(struct page *page) {
    page->flags &= ~(UINT32_MAX << NODE_ZONE_SHIFT);
}

static inline void set_page_nodezone(struct page *page, int nid, int zoneid) {
    clear_page_nodezone(page);
    page->flags |= NODEZONE(nid, zoneid) << NODE_ZONE_SHIFT;
}

extern struct zone *zone_table[];

static_assert(
    (1U << MAX_ZONE_SHIFT) >= MAX_NR_ZONES,
    "static_assert fail: MAX_ZONE_SHIFT is smaller then log2(MAX_NR_ZONES)\n");
static_assert((1 << MAX_NODE_SHIFT) >= MAX_NR_NODES,
              "static_assert fail: MAX_NODE_SHIFT is too small\n");
static_assert((1 << MAX_ZONE_SHIFT) >= MAX_NR_ZONES,
              "static_assert fail: MAX_ZONE_SHIFT is too small\n");

static inline uint32_t page_zone_id(struct page *page) {
    return (page->flags >> NODE_ZONE_SHIFT) & ~(UINT32_MAX << MAX_ZONE_SHIFT);
}

static inline struct zone *page_zone(struct page *page) {
    return zone_table[page->flags >> NODE_ZONE_SHIFT];
}

static inline uint32_t page_nid(struct page *page) {
    return page->flags >> (NODE_ZONE_SHIFT + MAX_ZONE_SHIFT);
}

static inline uint64_t page_pfn(struct page *page) {
    struct zone *zone = page_zone(page);
    return zone->start_pfn + (page - zone->mem_map);
}

#define page_node(page) NODE(page_nid((page)))

static inline uint32_t cpu_nodeid(uint32_t cpuid) {
    return 0;
}

static inline struct node *cpu_node(uint32_t cpuid) {
    return NODE(cpu_nodeid(cpuid));
}

static inline struct node *cpu_current_node() {
    return cpu_node(cpu_id());
}

#endif /* __MMZONE__ */

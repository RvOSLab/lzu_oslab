#ifndef __MMZONE__
#define __MMZONE__

#include <stddef.h>
#include <utils/linked_list.h>

#define MAX_GFP_ORDER 11
#define MAX_NR_NODES 1

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

#define PG_RESERVED 0U
#define PG_SLAB 1U
#define PG_LRU 2U

// Track the physical page status.
struct page {
    uint32_t flags;
    uint32_t count;
};

struct zone {
    struct page *mem_map;
    uint64_t start_pfn;
    uint64_t present_pages;
    uint64_t spanned_pages;

    struct free_area free_areas[MAX_GFP_ORDER];
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
    struct zonelist *zonelist[0]; // TODO(kongjun18): build zonelist

    int node_id;
};

// Currently only support one node. Thus always return init_node.
#define NODE(nid) (&init_node)

#endif /* __MMZONE__ */

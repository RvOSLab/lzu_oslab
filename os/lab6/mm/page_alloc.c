#include <stddef.h>
#include <mm.h>
#include <mmzone.h>
#include <page_alloc.h>

static inline void set_area_order(struct page *area, uint32_t order) {
    area->flags |= PG_PRIVATE;
    area->private = order;
}

static inline void rm_area_order(struct page *area) {
    area->flags &= ~PG_PRIVATE;
    area->private = 0;
}

static int check_area_allocatable(struct page *area, uint32_t order) {
    return !(area->flags & PG_RESERVED) && (area->count == 0) &&
           (area->flags & PG_PRIVATE) && (area->private == order);
}

static inline int check_area_nonallocated(struct page *area) {
    return !(area->flags & PG_RESERVED) && (area->count == 0) &&
           !(area->flags & PG_PRIVATE) && (area->private == 0);
}

static int check_area_allocated(struct page *area) {
    return !(area->flags & PG_RESERVED) && (area->count > 0) &&
           !(area->flags & PG_RESERVED) && (area->private == 0);
}

// area->lru is initialized in system startup
static inline void init_free_area(struct page *area, uint32_t order) {
    set_area_order(area, order);
    area->count = 0;
}

struct page *__alloc_pages(struct zone *zone, uint32_t order) {
    assert(order <= MAX_GFP_ORDER);
    assert(zone != NULL);

    uint32_t current_order = order;
    struct page *area = NULL;
    for (; current_order <= MAX_GFP_ORDER; current_order++) {
        struct free_area *free_area = zone->free_areas + current_order;
        if (linked_list_empty(&free_area->free_list)) {
            continue;
        }

        struct page *area =
            container_of(free_area->free_list.next, struct page, lru);
        if (check_area_allocatable(area, current_order)) {
            panic("__alloc_pages: area is not allocatable.");
        }
        free_area->nr_free--;
        linked_list_remove(&area->lru);
        zone->free_pages -= 1U << order;
        area->count = 1;
        rm_area_order(area);
        while (current_order-- > order) {
            free_area--;
            uint64_t area_size = 1 << current_order;
            struct page *buddy = area + area_size;
            assert(check_area_nonallocated(buddy));
            init_free_area(buddy, current_order);
            free_area->nr_free++;
            linked_list_push(&free_area->free_list, &buddy->lru);
        }
        break;
    }
    return area;
}

// Free `count` areas linked in `list` in `zone`
__unused static void __free_pages_in_bulk(struct zone *zone, uint32_t count,
                                          struct linked_list_node *list,
                                          uint32_t order) {
}

static inline int check_range(struct zone *zone, struct page *area,
                              uint32_t order) {
    return (zone == page_zone(area)) && (page_pfn(area) >= zone->start_pfn) &&
           ((page_pfn(area) + (1U << order)) <
            zone->start_pfn + zone->spanned_pages);
}

void __free_pages(struct page *page, uint32_t order) {
    if (page->count < 0) {
        panic("__free_pages: negative count");
    }

    if (!(check_area_allocated(page) && !(--page->count))) {
        return;
    }

    struct zone *zone = page_zone(page);
    assert(zone->mem_map <= page);
    uint64_t size = 1UL << order;
    zone->free_pages += size;
    uint64_t base_idx = page - zone->mem_map;
    if (base_idx & (size - 1)) {
        panic("__free_pages: not aligned");
    }

    for (; order <= MAX_GFP_ORDER - 1; order++) {
        uint64_t buddy_idx = base_idx ^ (1UL << order);
        struct page *buddy = zone->mem_map + buddy_idx;
        if (!check_range(zone, buddy, order)) {
            break;
        }
        if (!check_area_allocatable(buddy, order)) {
            break;
        }
        linked_list_remove(&buddy->lru);
        zone->free_areas[order].nr_free--;
        rm_area_order(buddy);
        base_idx &= buddy_idx;
    }
    struct page *coalesced = zone->mem_map + base_idx;
    init_free_area(coalesced, order);
    linked_list_push(&zone->free_areas[order].free_list, &coalesced->lru);
    zone->free_areas[order].nr_free++;
}

// Allocate `count` areas linked in `list`.
static inline uint32_t __alloc_pages_bulk(struct zone *zone, uint32_t order,
                                          uint32_t count,
                                          struct linked_list_node *list) {
    uint32_t allocated = 0;
    disable_interrupt();
    for (uint32_t i = 0; i < count; ++i) {
        struct page *page = __alloc_pages(zone, order);
        if (!page) {
            break;
        }
        ++allocated;
        linked_list_push(list, &page->lru);
    }
    enable_interrupt();
    return allocated;
}

struct page *alloc_pages_node(struct node *node, uint32_t order, gfp_t flags) {
    uint32_t target_zone = flags & GFP_ZONE_MASK;
    struct zone_list *zone_list = node->zone_lists + (flags & GFP_ZONE_MASK);
    struct zone *zone = &node->zones[target_zone];
    struct page *page = NULL;
    for (; zone_list; ++zone_list) {
        page = __alloc_pages(zone, order);
        if (page) {
            break;
        }
    }
    return page;
}

struct page *alloc_pages(uint32_t order, gfp_t flags) {
    return alloc_pages_node(cpu_current_node(), order, flags);
}

struct page *alloc_page(gfp_t flags) {
    return alloc_pages(0, flags);
}

void *get_free_pages(uint32_t order) {
    struct page *page = alloc_pages(order, GFP_KERNEL);
    if (!page) {
        return page;
    }
    return (void *)page_to_va(page);
}

void *get_free_page() {
    return get_free_pages(0);
}

void free_pages(struct page *page, uint32_t order) {
    return __free_pages(page, order);
}

__unused static void print_free_areas(struct free_area *free_areas) {
    for (int i = 0; i <= MAX_GFP_ORDER; i++) {
        kprintf("order %x: %x free areas\n", i, free_areas[i].nr_free);
    }
}

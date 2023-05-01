#include <asm/page.h>
#include <assert.h>
#include <bitops.h>
#include <compiler.h>
#include <device/fdt.h>
#include <page_alloc.h>
#include <kdebug.h>
#include <limits.h>
#include <mm.h>
#include <mmzone.h>
#include <sched.h>
#include <stddef.h>

struct zone *zone_table[1 << (MAX_NODE_SHIFT + MAX_ZONE_SHIFT)];

struct mem_areas mem_areas __initdata;

// TODO: move to shced.c
struct mm init_mm = { .pgd = boot_pg_dir };

// Currenty, only support one node.
struct bootmem init_bootmem;
struct node init_node = { .node_id = 0, .bootmem = &init_bootmem };
struct node *init_nodes[MAX_NR_NODES] = { &init_node };

// start and end of RAM
uint64_t ram_start_pfn;
uint64_t ram_end_pfn;
// start and end of physical address space
uint64_t start_pfn;
uint64_t end_pfn;

uint64_t present_pages; // available RAM pages

#define MAX_MEM_AREARS 8

enum mem_area_type {
    MEM_AREA_RAM,
    MEM_AREA_MMIO,
    MEM_AREA_DMA,
    MEM_AREA_TYPE_NR,
};

struct mem_area {
    uint64_t addr;
    uint64_t size;
    uint8_t type;
};

struct mem_areas {
    int8_t area_nr;
    struct mem_area areas[MAX_MEM_AREARS];
};

extern struct mem_areas mem_areas;

static void print_mem_areas() __init {
    for (int i = 0; i < mem_areas.area_nr; i++) {
        uint64_t addr = mem_areas.areas[i].addr;
        kprintf("mem area %x: [%x - %x]\n", i, addr,
                addr + mem_areas.areas[i].size);
    }
}

// Only support `MEM_AREA_RAM` for now.
void probe_mem_areas(const struct fdt_header *fdt) __init {
    struct fdt_node_header *root_node = fdt_find_node_by_path(fdt, "/");

    if (!root_node) {
        panic("/ not found");
    }

    struct fdt_property *address_cells_prop =
        fdt_get_prop(fdt, root_node, "#address-cells");
    struct fdt_property *size_cells_prop =
        fdt_get_prop(fdt, root_node, "#size-cells");
    if (!address_cells_prop || !size_cells_prop) {
        panic("#address-cells or #size-cells not found ");
    }
    uint32_t address_cells = fdt_get_prop_num_value(address_cells_prop, 0);
    uint32_t size_cells = fdt_get_prop_num_value(size_cells_prop, 0);

    // First "memory" node
    union fdt_walk_pointer memory_pointer = { .node = fdt_find_node_by_path(
                                                  fdt, "/memory") };
    while (1) {
        if (!memory_pointer.address)
            break;
        struct fdt_property *device_name_prop =
            fdt_get_prop(fdt, memory_pointer.node, "device_type");
        const char *device_name = fdt_get_prop_str_value(device_name_prop, 0);
        kprintf("[fdt] find %s at physical address %p\n", device_name,
                memory_pointer.node);
        struct fdt_property *reg_prop =
            fdt_get_prop(fdt, memory_pointer.node, "reg");
        uint32_t idx = 0;
        uint64_t address = 0;
        for (uint32_t i = 0; i < address_cells; i += 1) {
            address <<= 32;
            address |= fdt_get_prop_num_value(reg_prop, idx);
            idx += 1;
        }
        uint64_t length = 0;
        for (uint32_t i = 0; i < size_cells; i += 1) {
            length <<= 32;
            length |= fdt_get_prop_num_value(reg_prop, idx);
            idx += 1;
        }
        assert(strcmp(device_name, "memory") == 0,
               "expected device name: %s\ngot: %s\n", "memory", device_name);
        if (mem_areas.area_nr == MAX_MEM_AREARS) {
            break;
        }
        mem_areas.areas[mem_areas.area_nr++] = (struct mem_area){
            .addr = address, .size = length, .type = MEM_AREA_RAM
        };
        while (1) { // Jump to next node
            uint32_t deepth = fdt_skip_node(&memory_pointer);
            if (deepth) { // Incomplete device tree
                memory_pointer.address = 0;
                break;
            }
            if (!memory_pointer.address)
                break; // Finish
            if (memory_pointer.node->tag != FDT_BEGIN_NODE) {
                memory_pointer.address = 0;
                break;
            }
            const char *node_name = memory_pointer.node->name;
            const char *search = "memory";
            const char *search_end = search + strlen(search);
            while (search != search_end && *search == *node_name) {
                search += 1;
                node_name += 1;
            }
            if (search == search_end)
                break;
        }
    }
    print_mem_areas();
}

static void __init cal_phys_start_end() {
    ram_start_pfn = UINT64_MAX;
    start_pfn = UINT64_MAX;
    for (int i = 0; i < mem_areas.area_nr; i++) {
        struct mem_area *area = &mem_areas.areas[i];
        start_pfn = (start_pfn > area->addr) ? area->addr : start_pfn;
        end_pfn = (end_pfn < area->addr + area->size) ?
                      area->addr + area->size :
                      end_pfn;
        if (area->type == MEM_AREA_RAM) {
            present_pages += area->size;
            ram_start_pfn =
                (ram_start_pfn > area->addr) ? area->addr : ram_start_pfn;
            ram_end_pfn = (ram_end_pfn < area->addr + area->size) ?
                              area->addr + area->size :
                              ram_end_pfn;
        }
    }
    if (ram_start_pfn == UINT64_MAX || start_pfn == UINT64_MAX) {
        kprintf("can't find ram area! ");
        panic("ram_start_pfn: %x, start_pfn: %x\n", ram_start_pfn, start_pfn);
    }
    kprintf("RAM: [%x - %x]\n", ram_start_pfn, ram_end_pfn);
    kprintf("MEM: [%x - %x]\n", start_pfn, end_pfn);
    ram_start_pfn = CEIL(ram_start_pfn, PAGE_SIZE) >> PAGE_SHIFT;
    ram_end_pfn = FLOOR(ram_end_pfn, PAGE_SIZE) >> PAGE_SHIFT;
    start_pfn >>= PAGE_SHIFT;
    end_pfn >>= PAGE_SHIFT;
    present_pages >>= PAGE_SHIFT;
}

// Check if target area [*addrp, *addrp+size] is available
//
// find_free_area() is only used to allocate `boot_mem_map`, so only need
// to judge Whether the region overlaps with the kernel image and SBI.
static int __init bad_area(uint64_t *addrp, uint64_t size) {
    uint64_t end = (uint64_t)pa(&__end);
    if (*addrp < end) {
        *addrp = end;
        return 1;
    }
    return 0;
}

// Find free area in physical address range [`start`, `end`]
//
// If fail, find_free_area() will return `end`.
//
// How to avoid overlapping with allocated memory?
// 1. Carefully design args [start, end) to avoid overlapping.
// 2. Hardcode allocated memory areas in `bad_area()`.
uint64_t __init find_free_area(uint64_t start, uint64_t end, uint64_t size) {
    for (int i = 0; i < mem_areas.area_nr; i++) {
        struct mem_area *area = &mem_areas.areas[i];
        uint64_t addr = area->addr;
        if (addr < start)
            addr = start;
        if (addr > area->addr + area->size)
            continue;
        while (bad_area(&addr, size) &&
               (addr + size < area->addr + area->size)) {
        }
        uint64_t last = addr + size;
        if (last > area->addr + area->size)
            continue;
        if (last > end)
            continue;
        return addr;
    }
    return end;
}

void __init clear_bss() {
    memset(&__bss, 0, &__bss_end - &__bss + 1);
}

static void __init init_direct_mapping() {
    uint64_t map_start = va(FLOOR((start_pfn << PAGE_SHIFT), 1UL << 30));
    uint64_t map_end = va(CEIL(end_pfn << PAGE_SHIFT, 1UL << 30));
    while (map_start < map_end) {
        kprintf("build direct mapping: %x -> %x\n", map_start, pa(map_start));
        *pgd_entry_kernel(map_start) = mkpgd(pa(map_start), PROT_LEAF);
        map_start += (1UL << 30);
    }
}

static uint8_t *__init alloc_bootmap(uint64_t start_pfn, uint64_t end_pfn) {
    uint64_t pages = end_pfn - start_pfn + 1;
    uint64_t map_size = (pages + 7) / 8;
    uint8_t *bootmap =
        (uint8_t *)find_free_area(0, end_pfn << PAGE_SHIFT, map_size);
    if ((uint64_t)bootmap == (end_pfn << PAGE_SHIFT)) {
        panic("bootmem_init: can't find enough memory");
    }
    bootmap = (uint8_t *)va(bootmap);
    memset(bootmap, 0xff, map_size);
    return bootmap;
}

// start_pfn and end_pfn present the range of allocatable memory.
static void __init bootmem_init_node(int nid, uint64_t start_pfn,
                                     uint64_t end_pfn) {
    NODE(nid)->start_pfn = start_pfn;
    NODE(nid)->spanned_pages = end_pfn - start_pfn;
    NODE(nid)->present_pages = present_pages;
}

// Allocate `size` bytes of memory aligned to `align` from `bootmem`.
//
// `align` must be a power of 2. Currently, `bootmem_alloc()` always
// allocates memory aligned to PAGE_SIZE.
//
// I will implement align smaller than PAGE_SIZE in the future.
static void *__init bootmem_alloc(struct bootmem *bootmem, uint32_t size,
                                  uint32_t align) {
    assert((bootmem != NULL) && size && align && pow_of_2(align),
           "bootmem_alloc: bad param!");
    uint64_t mem_start = bootmem->start_pfn << PAGE_SHIFT;
    uint64_t area_size = (size + (PAGE_SIZE - 1)) / PAGE_SIZE;

    uint32_t offset = 0;
    if (mem_start != ALIGN(mem_start, align)) {
        offset = ALIGN(mem_start, align) - mem_start;
        offset >>= PAGE_SHIFT;
    }
    uint32_t eidx = bootmem->end_pfn - bootmem->start_pfn + 1;
    uint32_t step = (align >> PAGE_SHIFT) ?: 1;

    int64_t found_start = -1;

    for (uint32_t i = offset; i < eidx; i += step) {
        i = find_next_zero_bit(bootmem->mem_map, eidx, i);
        if (i == -1) {
            return NULL;
        }
        i = ALIGN(i, step);
        if (test_bit(bootmem->mem_map, i)) {
            continue;
        }
        uint32_t j;
        for (j = i + 1; j < i + area_size; ++j) {
            if (j >= eidx) {
                return NULL;
            }
            if (test_bit(bootmem->mem_map, j)) {
                break;
            }
        }
        if (j >= i + area_size) {
            found_start = i;
            break;
        }
        i = ALIGN(j, step);
    }

    if (found_start == -1) {
        return NULL;
    }

    for (uint32_t i = found_start; i < found_start + area_size; ++i) {
        set_bit(bootmem->mem_map, i);
    }

    void *addr = (void *)va((bootmem->start_pfn + found_start) << PAGE_SHIFT);
    memset(addr, 0, area_size << PAGE_SHIFT);
    return addr;
}

static void __init bootmem_free(struct bootmem *bootmem, uint64_t addr,
                                uint32_t size) {
    uint64_t i = (CEIL(pa(addr), PAGE_SIZE) >> PAGE_SHIFT) - bootmem->start_pfn;
    uint64_t end = ((pa(addr) + size) >> PAGE_SHIFT) - bootmem->start_pfn;
    for (; i < end; ++i) {
        if (!test_and_clear_bit(bootmem->mem_map, i)) {
            panic("bootmem_free: double free!");
        }
    }
}

// Mark virtual address range [start, end) reserved in bootmem allocator.
static void __init reserve_bootmem(struct bootmem *bootmem, uint64_t start,
                                   uint64_t end) {
    uint64_t start_pfn = start >> PAGE_SHIFT;
    uint64_t end_pfn = end >> PAGE_SHIFT;
    assert(bootmem->start_pfn <= start_pfn);
    assert(bootmem->end_pfn >= end_pfn);

    for (; start_pfn < end_pfn; ++start_pfn) {
        if (test_and_set_bit(bootmem->mem_map,
                             start_pfn - bootmem->start_pfn)) {
            panic("reserve_bootmem: page frame %x is not free", start_pfn);
        }
    }
}

// Free all physical memory into bootmem allocator.
static void __init free_init_mem_areas() {
    for (int i = 0; i < mem_areas.area_nr; i++) {
        struct mem_area *area = &mem_areas.areas[i];
        if (area->type != MEM_AREA_RAM) {
            continue;
        }
        assert((ram_end_pfn << PAGE_SHIFT) >= (area->addr + area->size),
               "ram_end_pfn corrupts");
        assert((ram_start_pfn << PAGE_SHIFT) <= area->addr,
               "ram_start_pfn corrupts");
        bootmem_free(NODE(0)->bootmem, va(area->addr), area->size);
    }
    uint64_t map_size =
        (NODE(0)->bootmem->end_pfn - NODE(0)->bootmem->start_pfn + 7) / 8;
    for (uint64_t i = 0; i < map_size; i++) {
        if (test_bit(NODE(0)->bootmem->mem_map, i)) {
            panic("ooooops");
        }
    }
}

static void __init reserve_kernel_data() {
    reserve_bootmem(NODE(0)->bootmem, pa(__START_KERNEL), pa(&__end));

    struct bootmem *bootmem = NODE(0)->bootmem;
    uint64_t map_size = (bootmem->end_pfn - bootmem->start_pfn + 7) / 8;
    reserve_bootmem(NODE(0)->bootmem, pa(bootmem->mem_map),
                    pa(bootmem->mem_map + map_size));
}

static void __init reserve_sbi_data() {
    reserve_bootmem(NODE(0)->bootmem, ram_start_pfn << PAGE_SHIFT,
                    MAX_SBI_ADDR);
}

static uint32_t __init __build_zonelists(struct node *node,
                                         struct zone_list *zone_list,
                                         uint32_t idx, uint32_t type) {
    switch (type) {
    case ZONE_NORMAL:
        zone_list->zones[idx++] = &node->zones[type];
    case ZONE_DMA:
        zone_list->zones[idx++] = &node->zones[type];
        break;
    default:
        panic("bad zone type");
    }
    return idx;
}

static void __init build_zonelists(struct node *node) {
    for (int i = 0; i < MAX_NR_ZONES; ++i) {
        uint32_t idx = 0;
        struct zone_list *zone_list = &node->zone_lists[i];
        memset(zone_list, 0, sizeof(*zone_list));
        uint32_t type = ZONE_NORMAL;
        if (i & GFP_DMA) {
            type = ZONE_DMA;
        }
        idx = __build_zonelists(node, zone_list, idx, type);
        zone_list->zones[idx] = NULL;
    }
}

static void __init bootmem_init() {
    init_bootmem = (struct bootmem){ .start_pfn = ram_start_pfn,
                                     .end_pfn = ram_end_pfn,
                                     .mem_map = alloc_bootmap(ram_start_pfn,
                                                              ram_end_pfn) };
    free_init_mem_areas();
    reserve_kernel_data();
    reserve_sbi_data();
    uint32_t map_size =
        (NODE(0)->bootmem->end_pfn - NODE(0)->bootmem->start_pfn) *
        sizeof(struct page);
    NODE(0)->mem_map = bootmem_alloc(NODE(0)->bootmem, map_size, 64);
    assert(NODE(0)->mem_map != NULL, "bootmem_alloc(): no memory");
}

// Calculate size and holes of each zone in pages.
//
// Ignore holes for now, this causes wrong zone present pages, but does not
// affect memory allocation.
static void zone_size_init(uint64_t *zone_sizes, uint64_t *zone_holes) {
    uint64_t dma_pages = DMA_ZONE_SIZE >> PAGE_SHIFT;
    zone_sizes[ZONE_DMA] = dma_pages;
    if (NODE(0)->present_pages < dma_pages) {
        zone_sizes[ZONE_DMA] = NODE(0)->present_pages / 4;
    }
    zone_sizes[ZONE_NORMAL] = NODE(0)->present_pages - zone_sizes[ZONE_DMA];
    memset(zone_holes, 0, MAX_NR_ZONES);
}

static void __init free_area_init_node(struct node *node, int nid,
                                       uint64_t start_pfn, uint64_t *zone_sizes,
                                       uint64_t *zone_holes) {
    node->node_id = nid;
    node->start_pfn = start_pfn;
    uint64_t size = 0;
    uint64_t real_size = 0;
    for (int i = 0; i < MAX_NR_ZONES; ++i) {
        size += zone_sizes[i];
        real_size = size - zone_holes[i];
    }
    node->spanned_pages = size;
    node->present_pages = real_size;

    for (int zoneid = 0; zoneid < MAX_NR_ZONES; ++zoneid) {
        struct zone *zone = &node->zones[zoneid];
        zone->free_pages = 0;
        zone->spanned_pages = zone_sizes[zoneid];
        zone->present_pages = zone_sizes[zoneid] - zone_holes[zoneid];
        zone->node = node;
        zone->start_pfn =
            (zoneid > 0) ? ((zone - 1)->start_pfn + (zone - 1)->spanned_pages) :
                           node->start_pfn;
        zone->mem_map = node->mem_map + (zone->start_pfn - node->start_pfn);
        zone_table[NODEZONE(nid, zoneid)] = zone;

        for (int i = 0; i <= MAX_GFP_ORDER; ++i) {
            zone->free_areas[i].nr_free = 0;
            linked_list_init(&zone->free_areas[i].free_list);
        }

        struct page *zone_mem_map = zone->mem_map;
        for (uint64_t i = 0; i < zone->spanned_pages; ++i) {
            if (zone_mem_map[i].flags & PG_RESERVED) {
                panic("free_area_init_node: page %x is reserved",
                      i + zone->start_pfn);
            }
            memset(&zone_mem_map[i], 0, sizeof(*zone_mem_map));
            set_page_nodezone(&zone->mem_map[i], nid, zoneid);
            linked_list_init(&zone_mem_map[i].lru);
        }
    }
}

static void __init retire_bootmem() {
    bootmem_init_node(0, ram_start_pfn, ram_end_pfn);
    uint64_t zone_sizes[MAX_NR_ZONES] = { 0 };
    uint64_t zone_holes[MAX_NR_ZONES] = { 0 };
    zone_size_init(zone_sizes, zone_holes);
    free_area_init_node(NODE(0), 0, NODE(0)->start_pfn, zone_sizes, zone_holes);

    uint8_t *bootmap = NODE(0)->bootmem->mem_map;
    uint32_t block_size = 64;
    for (uint64_t i = 0; i < NODE(0)->spanned_pages; ++i) {
        assert(NODE(0)->mem_map[i].count == 0);
        if ((i + block_size <= NODE(0)->spanned_pages) &&
            (bootmap[__idx(i)] == 0)) {
            struct page *page = &NODE(0)->mem_map[i];
            page->count = 1;
            for (int j = i; j < i + 64; ++j, ++page) {
                page->flags &= ~PG_RESERVED;
            }
            free_pages(&NODE(0)->mem_map[i], 6);
            i += 63;
        } else if (bootmap[__idx(0)] != 0xffU) {
            if (!test_bit(bootmap, i)) {
                struct page *page = &NODE(0)->mem_map[i];
                page->flags &= ~PG_RESERVED;
                page->count = 1;
                free_pages(page, 0);
            }
        } else {
            i += 63;
        }
    }

    // Reclaim bootmap
    struct page *page = va_to_page(bootmap);
    uint32_t eidx =
        ((NODE(0)->bootmem->end_pfn - NODE(0)->bootmem->start_pfn + 7) / 8 +
         PAGE_SIZE - 1) /
        PAGE_SIZE;
    for (uint32_t i = 0; i < eidx; ++i, ++page) {
        page->flags &= ~PG_RESERVED;
        page->count = 1;
        free_pages(page, 0);
    }
    NODE(0)->bootmem = NULL;

    build_zonelists(NODE(0));
}

void mem_init() {
    cal_phys_start_end();
    init_direct_mapping();
    bootmem_init();
    retire_bootmem();
}

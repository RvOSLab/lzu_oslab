#include <mm.h>
#include <device.h>

#define DRIVER_MEM_START 0xCC000000
uint64_t mem_resource_ptr = DRIVER_MEM_START;

void mem_resource_map(struct driver_resource *res) {
    uint64_t map_start = FLOOR(res->resource_start);
    uint64_t map_end = CEIL(res->resource_end);
    res->map_address = mem_resource_ptr;
    while (map_start < map_end) {
        put_page(map_start, mem_resource_ptr, KERN_RW | PAGE_VALID);
        map_start += PAGE_SIZE;
        mem_resource_ptr += PAGE_SIZE;
    }
}

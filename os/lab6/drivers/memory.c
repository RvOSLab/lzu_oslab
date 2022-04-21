#include <mm.h>
#include <device.h>
#include <assert.h>

#define DRIVER_MEM_START 0xCC000000
struct linked_list_node mem_resource_list;

void init_mem_resource() {
    linked_list_init(&mem_resource_list);
}

void mem_resource_map(struct driver_resource *res) {
    uint64_t map_start = FLOOR(res->resource_start);
    uint64_t map_end = CEIL(res->resource_end);
    uint64_t map_length = map_end - map_start;

    uint64_t mem_resource_ptr = DRIVER_MEM_START;
    struct linked_list_node *node;
    for_each_linked_list_node(node, &mem_resource_list) {
        struct driver_resource *res_using = container_of(node, struct driver_resource, list_node);
        if (res_using == res) panic("重复使用资源描述符");
        if (res_using->map_address >= mem_resource_ptr) {
            uint64_t space = res_using->map_address - mem_resource_ptr;
            if (space >= map_length) break;
            mem_resource_ptr = (
                res_using->map_address +
                CEIL(res_using->resource_end) -
                FLOOR(res_using->resource_start)
            );
        } else {
            panic("重叠的设备MMIO内存分配项");
        }
    }

    res->map_address = mem_resource_ptr;
    while (map_start < map_end) {
        put_page(map_start, mem_resource_ptr, KERN_RW | PAGE_VALID);
        map_start += PAGE_SIZE;
        mem_resource_ptr += PAGE_SIZE;
    }

    linked_list_insert_before(node, &res->list_node);
}

void mem_resource_unmap(struct driver_resource *res) {
    free_page_tables(res->map_address, res->resource_end - res->resource_start);
    linked_list_remove(&res->list_node);
}

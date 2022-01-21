#include <device/fdt.h>
#include <device.h>
#include <kdebug.h>
#include <mm.h>

struct driver_resource fdt_mem = {
    .resource_type = DRIVER_RESOURCE_MEM
};

void fdt_print_node_props(const struct fdt_header *fdt, struct fdt_node_header *node, uint32_t level) {
    union fdt_walk_pointer pointer = { .node = node };
    pointer.prop = fdt_get_props_of_node(pointer.node);
    while (pointer.prop->tag == FDT_PROP) {
        for (uint32_t i = 0; i < level; i+=1) kprintf("  ");
        kputchar('-');
        kputs(fdt_get_prop_name(fdt, pointer.prop->name_offset));
        fdt_walk_prop(&pointer);
    }
}

void fdt_test(const struct fdt_header *fdt) {
    struct device *dev = kmalloc(sizeof(struct device));
    device_init(dev);
    fdt_mem.resource_start = (uint64_t)fdt;
    fdt_mem.resource_end = fdt_mem.resource_start + 4 * PAGE_SIZE;
    device_add_resource(dev, &fdt_mem);

    fdt = (const struct fdt_header *)fdt_mem.map_address;
    union fdt_walk_pointer pointer = { .address = (uint64_t)fdt };
    pointer.address += fdt32_to_cpu(fdt->off_dt_struct);

    struct fdt_node_header *parent = NULL;
    uint32_t deepth = 0;
    parent = fdt_walk_node(&pointer);
    while (pointer.address) {
        if (parent) deepth += 1;
        if(pointer.node->tag == FDT_BEGIN_NODE) {
            for (uint32_t i = 0; i < deepth; i+=1) kprintf("  ");
            kprintf("%s\n", pointer.node->name);
            fdt_print_node_props(fdt, pointer.node, deepth + 1);
        } else {
            deepth -= 1;
        }
        parent = fdt_walk_node(&pointer);
    }
    pointer.node = fdt_find_node(fdt, "/soc/virtio_mmio@10008000");
    pointer.prop = fdt_get_prop(fdt, pointer.node, "interrupts");
}

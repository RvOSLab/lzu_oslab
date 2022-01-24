#include <device/fdt.h>
#include <device.h>
#include <kdebug.h>
#include <mm.h>

struct driver_resource fdt_mem = {
    .resource_type = DRIVER_RESOURCE_MEM
};

void fdt_match_drivers_by_node(const struct fdt_header *fdt, struct fdt_node_header *node, struct device_driver *driver_list[]) {
    struct fdt_property *prop = fdt_get_prop(fdt, node, "compatible");
    if (!prop) return;
    // if (!device_list) return;
    uint32_t prop_used_len = 0;
    while (prop_used_len < fdt_get_prop_value_len(prop)) {
        const char *compatible = fdt_get_prop_str_value(prop, prop_used_len);
        kprintf("%s ", compatible);
        prop_used_len += strlen(compatible) + 1;
    }
    kputchar('\n');
}

void fdt_loader(const struct fdt_header *fdt) {
    struct device *dev = kmalloc(sizeof(struct device));
    device_init(dev);
    fdt_mem.resource_start = (uint64_t)fdt;
    fdt_mem.resource_end = fdt_mem.resource_start + 2 * PAGE_SIZE;
    device_add_resource(dev, &fdt_mem);

    fdt = (const struct fdt_header *)fdt_mem.map_address;
    union fdt_walk_pointer pointer = { .address = (uint64_t)fdt };
    pointer.address += fdt32_to_cpu(fdt->off_dt_struct);

    while (pointer.address) {
        if (pointer.node->tag == FDT_BEGIN_NODE) {
            fdt_match_drivers_by_node(fdt, pointer.node, NULL);
        }
        fdt_walk_node(&pointer);
    }
}

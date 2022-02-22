#include <device/virtio.h>
#include <device/virtio/virtio_mmio.h>

#include <device/fdt.h>
#include <device/irq.h>

#include <kdebug.h>
#include <mm.h>

uint64_t virtio_device_probe(struct device *dev) {
    device_init(dev);
    struct fdt_header *fdt = device_get_fdt(dev);
    struct fdt_node_header * node = device_get_fdt_node(dev);
    struct fdt_property *reg = fdt_get_prop(fdt, node, "reg");
    struct driver_resource *mem = (struct driver_resource *)kmalloc(sizeof(struct driver_resource));
    mem->resource_type = DRIVER_RESOURCE_MEM;
    mem->resource_start = fdt_get_prop_num_value(reg, 0) << sizeof(fdt32_t);
    mem->resource_start += fdt_get_prop_num_value(reg, 1);
    mem->resource_end = mem->resource_start;
    mem->resource_end += fdt_get_prop_num_value(reg, 2) << sizeof(fdt32_t);
    mem->resource_end += fdt_get_prop_num_value(reg, 3);
    device_add_resource(dev, mem);
    device_set_data(dev, mem);
    virtio_mmio_probe(dev);
    return 0;
}

struct driver_match_table virtio_match_table[] = {
    { .compatible = "virtio,mmio" },
    { NULL }
};

struct device_driver virtio_driver = {
    .driver_name = "MaPl VirtIO driver",
    .match_table = virtio_match_table,
    .device_probe = virtio_device_probe
};

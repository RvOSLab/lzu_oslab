#ifndef DEVICE_H
#define DEVICE_H

#include <stddef.h>
#include <utils/linked_list.h>
#include <utils/hash_table.h>
#include <device/fdt.h>

struct device {
    uint64_t ref_count;
    struct device *parent;
    struct hash_table_node hash_node;

    const char *device_name;
    uint64_t device_id;
    void *match_data;
    struct fdt_header *fdt;
    struct fdt_node_header *fdt_node;
    struct linked_list_node resource_list;

    void *driver_data;

    uint64_t interface_flag;
    void *(*get_interface)(struct device *, uint64_t interface_id);
};

struct driver_match_table {
    const char *compatible;
    void *match_data;
};

#define DRIVER_RESOURCE_MEM  1
struct driver_resource {
    uint64_t resource_start;
    uint64_t resource_end;
    uint64_t resource_type;
    uint64_t map_address;

    struct linked_list_node list_node;
};

struct device_driver {
    const char *driver_name;
    struct driver_match_table *match_table;

    uint64_t (*device_probe)(struct device *dev);
};

#define DEVICE_TABLE_BUFFER_LENGTH 11
extern struct hash_table device_table;
void init_device_table();
uint32_t device_table_get_major_num(uint32_t major);
uint32_t device_table_get_next_minor(uint32_t major, uint32_t minor_start);
struct device *get_dev_by_major_minor(uint32_t major, uint32_t minor);

uint64_t char_dev_test(uint64_t c);
uint64_t reset_dev_test(uint64_t function);
uint64_t block_dev_test();
uint64_t net_dev_test();

void mem_resource_map(struct driver_resource *res);

static inline void device_set_data(struct device *dev, void *data) {
    dev->driver_data = data;
}
static inline void *device_get_data(struct device *dev) {
    return dev->driver_data;
}
static inline void *device_get_match_data(struct device *dev) {
    return dev->match_data;
}
static inline struct fdt_header *device_get_fdt(struct device *dev) {
    return dev->fdt;
}
static inline struct fdt_node_header *device_get_fdt_node(struct device *dev) {
    return dev->fdt_node;
}
static inline void device_set_interface(struct device *dev, uint64_t flag, void *(*getter)(struct device *, uint64_t)) {
    dev->interface_flag = flag;
    dev->get_interface = getter;
}
static inline uint32_t device_get_major(struct device *dev) {
    return dev->device_id >> 32;
}
static inline uint32_t device_get_minor(struct device *dev) {
    return dev->device_id & 0xFFFFFFFF;
}
static inline uint64_t device_set_major(struct device *dev, uint32_t major) {
    return dev->device_id = ((uint64_t)major << 32) | device_get_minor(dev);
}
static inline uint64_t device_set_minor(struct device *dev, uint32_t minor) {
    return dev->device_id = ((uint64_t)minor & 0xFFFFFFFF) | device_get_major(dev);
}
static inline void device_init(struct device *dev) {
    dev->ref_count = 0;
    dev->parent = NULL;
    dev->driver_data = NULL;
    dev->interface_flag = 0;
    dev->get_interface = NULL;
    linked_list_init(&dev->resource_list);
}
static inline void device_register(struct device *dev, const char *name, uint32_t major, struct device *parent) {
    uint32_t minor = device_table_get_next_minor(major, 1);
    device_set_major(dev, major);
    device_set_minor(dev, minor);
    dev->device_name = name;
    dev->parent = parent;
    if (parent) dev->parent->ref_count += 1;
    hash_table_set(&device_table, &dev->hash_node);
}
static inline void device_add_resource(struct device *dev, struct driver_resource *res) {
    switch(res->resource_type) {
        case DRIVER_RESOURCE_MEM:
            mem_resource_map(res);
            break;
        default:
            // Do nothing
            break;
    }
    linked_list_push(&dev->resource_list, &res->list_node);
}
#endif

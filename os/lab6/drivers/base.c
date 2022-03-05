#include <device.h>

uint64_t device_table_get_hash(struct hash_table_node *node) {
    struct device *dev = container_of(node, struct device, hash_node);
    return (dev->device_id * 0x10001) + 19;
}

uint64_t device_table_is_equal(struct hash_table_node *nodeA, struct hash_table_node *nodeB) {
    struct device *devA = container_of(nodeA, struct device, hash_node);
    struct device *devB = container_of(nodeB, struct device, hash_node);
    return devA->device_id == devB->device_id;
}

struct hash_table_node device_table_buffer[DEVICE_TABLE_BUFFER_LENGTH];
struct hash_table device_table = {
    .buffer = device_table_buffer,
    .buffer_length = DEVICE_TABLE_BUFFER_LENGTH,
    .get_hash = device_table_get_hash,
    .is_equal = device_table_is_equal
};

void init_device_table() {
    hash_table_init(&device_table);
}

uint32_t device_table_get_major_num(uint32_t major) {
    uint64_t buffer_length = device_table.buffer_length;
    uint32_t num = 0;
    for (uint64_t i = 0; i < buffer_length; i += 1) {
        struct linked_list_node *list_node;
        struct hash_table_node *hash_node;
        struct device *dev;
        for_each_linked_list_node(list_node, &device_table.buffer[i].confliced_list) {
            hash_node = container_of(list_node, struct hash_table_node, confliced_list);
            dev = container_of(hash_node, struct device, hash_node);
            if (device_get_major(dev) == major) {
                num += 1;
            }
        }
    }
    return num;
}

uint32_t device_table_get_next_minor(uint32_t major, uint32_t minor_start) {
    struct device dev;
    struct hash_table_node *node;
    device_set_major(&dev, major);
    uint32_t minor = minor_start;
    while(1) {
        device_set_minor(&dev, minor);
        node = hash_table_get(&device_table, &dev.hash_node);
        if(!node) return minor;
        minor += 1;
    }
}

struct device *get_dev_by_major_minor(uint32_t major, uint32_t minor) {
    struct device dev;
    device_set_major(&dev, major);
    device_set_minor(&dev, minor);
    struct hash_table_node *node;
    node = hash_table_get(&device_table, &dev.hash_node);
    return container_of(node, struct device, hash_node);
}

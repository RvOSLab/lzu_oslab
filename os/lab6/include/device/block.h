#ifndef DEVICE_BLOCK_H
#define DEVICE_BLOCK_H

#include <stddef.h>
#include <sched.h>
#include <device.h>
#include <utils/hash_table.h>

#define BLOCK_INTERFACE_BIT (1 << 7)

struct block_request {
    uint64_t is_read;
    uint64_t sector;
    void *buffer;
    struct task_struct * wait_queue;
};

struct block_device {
    struct device *dev;
    void (*request)(struct device *dev, struct block_request *request);
};

#endif

#ifndef BLOCK_CACHE_H
#define BLOCK_CACHE_H

#include <device/block.h>
#include <utils/hash_table.h>

enum block_cache_request_flag {
    BLOCK_READ = 1,
    BLOCK_WRITE = 2,
    BLOCK_FLUSH = 3 // NOTE: for test
};

struct block_cache_request {
    uint64_t request_flag;
    uint64_t offset;
    uint64_t length;
    void *target;
};

enum block_cache_flag {
    BLOCK_CACHE_VAILD = 1,
    BLOCK_CACHE_DIRTY = 2,
    BLOCK_CACHE_BUSY = 4,
    BLOCK_CACHE_ERROR = 8
};

struct block_cache {
    volatile uint64_t status_flag;

    struct device *dev;
    uint64_t block_idx;

    char *buffer;

    struct hash_table_node hash_node;
    struct linked_list_node list_node;

    struct task_struct *wait_queue;
};

extern int64_t block_cache_length;
extern struct linked_list_node block_cache_list;

void block_cache_init();
void block_cache_clip();
int64_t block_cache_request(struct device *dev, struct block_cache_request *request);

#endif

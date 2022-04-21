#include <device/block/block_cache.h>

/* 块设备 缓存结构 开始 */
uint64_t block_cache_get_hash(struct hash_table_node *node) {
    struct block_cache *cache = container_of(node, struct block_cache, hash_node);
    return cache->block_idx * 0x10001 + 19 * (uint64_t)cache->dev;
}

uint64_t block_cache_is_equal(struct hash_table_node *nodeA, struct hash_table_node *nodeB) {
    struct block_cache *cacheA = container_of(nodeA, struct block_cache, hash_node);
    struct block_cache *cacheB = container_of(nodeB, struct block_cache, hash_node);
    return (
        cacheA->dev == cacheB->dev &&
        cacheA->block_idx == cacheB->block_idx
    );
}

#define BLOCK_CACHE_HASH_LENGTH 13
struct hash_table_node block_cache_buffer[BLOCK_CACHE_HASH_LENGTH];
struct hash_table block_cache_table = {
    .buffer = block_cache_buffer,
    .buffer_length = BLOCK_CACHE_HASH_LENGTH,
    .get_hash = block_cache_get_hash,
    .is_equal = block_cache_is_equal
};
struct linked_list_node block_cache_list;
int64_t block_cache_length = 10;
int64_t block_cached_num = 0;
/* VFS inode 缓存结构 结束 */

void block_cache_init() {
    block_cached_num = 0;
    hash_table_init(&block_cache_table);
    linked_list_init(&block_cache_list);
}

static struct block_cache *block_cache_alloc() {
    return (struct block_cache *)kmalloc(sizeof(struct block_cache));
}

static void block_cache_free(struct block_cache *cache) {
    kfree_s(cache, sizeof(struct block_cache));
}

static struct block_cache *block_cache_query(struct device *dev, uint64_t block_idx) {
    struct block_cache cache = {
        .dev = dev,
        .block_idx = block_idx
    };
    struct hash_table_node *node = hash_table_get(&block_cache_table, &cache.hash_node);
    if (!node) return NULL;
    struct block_cache *real_cache = container_of(node, struct block_cache, hash_node);
    return &real_cache;
}

int64_t block_cache_request(struct device *dev, struct block_cache_request *request) {
    // 依靠VAILD位实现不需要对设备加锁
}

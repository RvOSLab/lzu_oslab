#include <device/block/block_cache.h>
#include <assert.h>

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
/* 块设备 缓存结构 结束 */

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

/* 在块缓存中查找指定项, 找不到返回NULL, 不更新LRU链表 */
static struct block_cache *block_cache_query(struct device *dev, uint64_t block_idx) {
    struct block_cache cache = {
        .dev = dev,
        .block_idx = block_idx
    };
    struct hash_table_node *node = hash_table_get(&block_cache_table, &cache.hash_node);
    if (!node) return NULL;
    struct block_cache *real_cache = container_of(node, struct block_cache, hash_node);
    return real_cache;
}

static void block_cache_load(struct block_cache *cache) {
    struct device *dev = cache->dev;
    struct block_device *blk_dev = dev->get_interface(dev, BLOCK_INTERFACE_BIT);
    if (!cache->buffer) {
        cache->buffer = kmalloc(blk_dev->block_size);
    }
    if (cache->status_flag & BLOCK_CACHE_DIRTY) {
        panic("load before store dirty");
    }
    struct block_request req = {
        .is_read = 1,
        .sector = cache->block_idx,
        .buffer = cache->buffer,
        .wait_queue = NULL
    };
    int64_t ret = blk_dev->request(dev, &req);
    if (ret < 0) cache->status_flag |= BLOCK_CACHE_ERROR;
    cache->status_flag |= BLOCK_CACHE_VAILD;
}

static void block_cache_store(struct block_cache *cache) {
    struct device *dev = cache->dev;
    struct block_device *blk_dev = dev->get_interface(dev, BLOCK_INTERFACE_BIT);
    if (!cache->buffer || !(cache->status_flag & BLOCK_CACHE_VAILD)) {
        panic("store before load vaild");
    }
    if (cache->status_flag & BLOCK_CACHE_DIRTY) {
        struct block_request req = {
            .is_read = 0,
            .sector = cache->block_idx,
            .buffer = cache->buffer,
            .wait_queue = NULL
        };
        int64_t ret = blk_dev->request(dev, &req);
        if (ret < 0) cache->status_flag |= BLOCK_CACHE_ERROR;
        cache->status_flag &= ~BLOCK_CACHE_DIRTY;
    }
}

void block_cache_clip() {
    while (block_cached_num >= block_cache_length) {
        struct linked_list_node *node = linked_list_pop(&block_cache_list);
        if (!node) break;
        struct block_cache *cache = container_of(node, struct block_cache, list_node);
        if (cache->status_flag & BLOCK_CACHE_BUSY) panic("try to free busy block cache");
        /* 从哈希表中移除, 防止其他进程找到 */
        hash_table_del(&block_cache_table, &cache->hash_node);
        linked_list_remove(&cache->list_node);
        block_cached_num -= 1;
        block_cache_store(cache);
        kfree(cache->buffer);
        block_cache_free(cache);
    }
}

/* 获取指定设备和块号的块缓冲, 找不到则新建缓冲区, 更新LRU链表 */
static struct block_cache *block_cache_get(struct device *dev, uint64_t block_idx) {
    struct block_cache *cache = block_cache_query(dev, block_idx);
    if (!cache) { // 没有对应缓冲区时新建
        block_cache_clip();
        cache = block_cache_alloc();
        if (!cache) return NULL;
        cache->status_flag = 0; // 状态标志为0
        cache->dev = dev;
        cache->block_idx = block_idx;
        cache->buffer = NULL;
        cache->wait_queue = NULL;
        hash_table_set(&block_cache_table, &cache->hash_node);
        block_cached_num += 1;
    } else {
        // 从LRU链表摘下该缓冲区, 防止在其他进程中的block_cache_get里被释放
        linked_list_remove(&cache->list_node);
    }
    /* 互斥锁 开始 */
    while (cache->status_flag & BLOCK_CACHE_BUSY) { // 等待在该块缓冲上的其他IO操作执行结束
        sleep_on(&cache->wait_queue);
    }
    cache->status_flag |= BLOCK_CACHE_BUSY;
    /* 互斥锁 解锁: 设置status_flag、放回LRU链表、wake_up*/
    return cache;
}

int64_t block_cache_request(struct device *dev, struct block_cache_request *request) {
    /* 检查是否为块设备, 请求标志、目标内存指针是否设置 */
    if (!(dev->interface_flag & BLOCK_INTERFACE_BIT)) return -ENOTBLK;
    if (!request->request_flag || !request->target) return -EINVAL;
    struct block_device *blk_dev = dev->get_interface(dev, BLOCK_INTERFACE_BIT);
    if (!blk_dev) return -EFAULT;

    /* 检查起始位置(offset)是是否超出块设备范围 */
    uint64_t block_device_size = blk_dev->block_num * blk_dev->block_size;
    if (request->offset >= block_device_size) return 0;
    uint64_t blk_index = request->offset / blk_dev->block_size;
    uint64_t blk_offset = request->offset % blk_dev->block_size;
    /* 检查读取长度(length)是否超出块设备范围 */
    uint64_t real_bytes_len = request->length;
    if (real_bytes_len > block_device_size - request->offset) {
        real_bytes_len = block_device_size - request->offset;
    }

    /* 初始化拷贝计数器和目标内存指针 */
    uint64_t bytes_counter = 0;
    char *dest_buffer = request->target;
    do {
        /* 获取并加载指定块缓冲区 */
        struct block_cache *cache = block_cache_get(dev, blk_index);
        if (!(cache->status_flag & BLOCK_CACHE_VAILD)) {
            block_cache_load(cache);
        }
        if (cache->status_flag & BLOCK_CACHE_ERROR) return -EIO;
        /* 计算当前块缓冲区需要拷贝的长度 */
        uint64_t cpy_len = blk_dev->block_size - blk_offset;
        if ((real_bytes_len - bytes_counter) < cpy_len) {
            cpy_len = real_bytes_len - bytes_counter;
        }
        /* 搬移数据 */
        if (request->request_flag & BLOCK_READ) {
            if (request->request_flag & BLOCK_FLUSH) {
                block_cache_store(cache);
                if (cache->status_flag & BLOCK_CACHE_ERROR) return -EIO;
            }
            memcpy(&dest_buffer[bytes_counter], &cache->buffer[blk_offset], cpy_len);
        } else {
            memcpy(&cache->buffer[blk_offset], &dest_buffer[bytes_counter], cpy_len);
            cache->status_flag |= BLOCK_CACHE_DIRTY;
            if (request->request_flag & BLOCK_FLUSH) {
                block_cache_store(cache);
                if (cache->status_flag & BLOCK_CACHE_ERROR) return -EIO;
            }
        }
        /* 解锁缓冲区 */
        cache->status_flag &= ~BLOCK_CACHE_BUSY;
        linked_list_unshift(&block_cache_list, &cache->list_node);
        wake_up(&cache->wait_queue);
        /* 更新拷贝计数器和块缓冲偏移 */
        bytes_counter += cpy_len;
        blk_offset = 0;
        blk_index += 1;
    } while (bytes_counter < real_bytes_len);
    return bytes_counter;
}

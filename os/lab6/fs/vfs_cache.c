#include <fs/vfs_cache.h>

/* VFS inode 缓存结构 开始 */
uint64_t vfs_inode_cache_get_hash(struct hash_table_node *node) {
    struct vfs_inode_cache *cache = container_of(node, struct vfs_inode_cache, hash_node);
    return cache->inode.inode_idx * 0x10001 + 19 * (uint64_t)cache->inode.fs;
}

uint64_t vfs_inode_cache_is_equal(struct hash_table_node *nodeA, struct hash_table_node *nodeB) {
    struct vfs_inode_cache *cacheA = container_of(nodeA, struct vfs_inode_cache, hash_node);
    struct vfs_inode_cache *cacheB = container_of(nodeB, struct vfs_inode_cache, hash_node);
    return (
        cacheA->inode.fs == cacheB->inode.fs &&
        cacheA->inode.inode_idx == cacheB->inode.inode_idx
    );
}

#define VFS_INODE_CACHE_HASH_LENGTH 13
struct hash_table_node vfs_inode_cache_buffer[VFS_INODE_CACHE_HASH_LENGTH];
struct hash_table vfs_inode_cache_table = {
    .buffer = vfs_inode_cache_buffer,
    .buffer_length = VFS_INODE_CACHE_HASH_LENGTH,
    .get_hash = vfs_inode_cache_get_hash,
    .is_equal = vfs_inode_cache_is_equal
};
struct linked_list_node vfs_inode_cache_list;
int64_t vfs_inode_cache_length = 0;
int64_t vfs_inode_cached_num = 0;
/* VFS inode 缓存结构 结束 */

void vfs_inode_cache_init() {
    vfs_inode_cached_num = 0;
    hash_table_init(&vfs_inode_cache_table);
    linked_list_init(&vfs_inode_cache_list);
}

struct vfs_inode *vfs_inode_cache_queue(struct vfs_instance *fs, uint64_t inode_idx) {
    struct vfs_inode_cache cache = { // 根据 fs 和 inode_idx 查找缓存项
        .inode = {
            .fs = fs,
            .inode_idx = inode_idx
        }
    };
    struct hash_table_node *node = hash_table_get(&vfs_inode_cache_table, &cache.hash_node);
    if (!node) return NULL;
    struct vfs_inode_cache *real_cache = container_of(node, struct vfs_inode_cache, hash_node);
    return &real_cache->inode;
}

struct vfs_inode_cache *vfs_inode_cache_alloc() {
    return (struct vfs_inode_cache *)kmalloc(sizeof(struct vfs_inode_cache));
}

void vfs_inode_cache_free(struct vfs_inode_cache *cache) {
    kfree(cache, sizeof(struct vfs_inode_cache));
}

void vfs_inode_cache_add(struct vfs_inode_cache *cache) {
    hash_table_set(&vfs_inode_cache_table, &cache->hash_node);
}

void vfs_inode_cache_del(struct vfs_inode_cache *cache) {
    hash_table_del(&vfs_inode_cache_table, &cache->hash_node);
}

void vfs_inode_cache_put_unused(struct vfs_inode *inode) {
    // TODO: clip
    struct vfs_inode_cache *cache = container_of(inode, struct vfs_inode_cache, inode);
    linked_list_unshift(&vfs_inode_cache_list, &cache->hash_node);
    vfs_inode_cached_num += 1;
}

void vfs_inode_cache_del_unused(struct vfs_inode *inode) {
    struct vfs_inode_cache *cache = container_of(inode, struct vfs_inode_cache, inode);
    linked_list_remove(&cache->list_node);
    vfs_inode_cached_num -= 1;
}

void vfs_inode_cache_see_unused(struct vfs_inode *inode) {
    struct vfs_inode_cache *cache = container_of(inode, struct vfs_inode_cache, inode);
    linked_list_remove(&cache->list_node);
    linked_list_unshift(&vfs_inode_cache_list, &cache->hash_node);
}
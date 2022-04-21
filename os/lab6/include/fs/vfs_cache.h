#ifndef VFS_CACHE_H
#define VFS_CACHE_H

#include <fs/vfs.h>
#include <utils/hash_table.h>
#include <mm.h>

struct vfs_inode_cache {
    struct vfs_inode inode;
    struct hash_table_node hash_node;
    struct linked_list_node list_node;
};

void vfs_inode_cache_init();

struct vfs_inode *vfs_inode_cache_query(struct vfs_instance *fs, uint64_t inode_idx);

struct vfs_inode_cache *vfs_inode_cache_alloc();
void vfs_inode_cache_free(struct vfs_inode_cache *cache);

void vfs_inode_cache_add(struct vfs_inode_cache *cache);

void vfs_inode_cache_put_unused(struct vfs_inode *inode);
void vfs_inode_cache_del_unused(struct vfs_inode *inode);
void vfs_inode_cache_see_unused(struct vfs_inode *inode);

#endif

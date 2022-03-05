#ifndef VFS_H
#define VFS_H

#include <stddef.h>

struct vfs_inode;
struct vfs_stat;

struct vfs_interface {
    void *fs_data;
    struct vfs_inode *root;
    void (*init_fs)(struct vfs_interface *fs);
    struct vfs_inode *(*open_inode)(struct vfs_inode *inode);
    void (*close_inode)(struct vfs_inode *inode);
    struct vfs_stat *(*get_stat)(struct vfs_inode *inode);
    uint64_t (*is_dir)(struct vfs_inode *inode);
    struct vfs_dir_entry *(*dir_inode)(struct vfs_inode *inode, uint64_t dir_idx);
    void (*inode_request)(struct vfs_inode *inode, void *buffer, uint64_t length, uint64_t offset, uint64_t is_read);
    uint64_t ref_cnt;
};

struct vfs_stat {
    // uint64_t type;
    uint64_t size;
    uint64_t time;
    uint64_t gid;
    uint64_t uid;
};

struct vfs_inode {
    void *inode_data;
    uint64_t inode_idx;
    struct vfs_interface *fs;
    uint64_t ref_cnt;
};

struct vfs_dir_entry {
    const char *name;
    uint64_t inode_idx;
};

/* 其他子系统要持有vfs_inode必须调用vfs_ref_inode，否则可能inode资源会被自动释放 */
/* 无论是否持有，使用完inode后必须调用vfs_free_inode，系统会自动释放资源 */
/* TODO: vfs_new_inode 已新建inode缓冲区 */
/* TODO: vfs_free_inode 懒释放缓冲区 */

/* 从指定文件系统中提取inode */
struct vfs_inode *vfs_new_inode(struct vfs_interface *fs, uint64_t inode_idx);

/* vfs提供的引用计数管理操作 */
void vfs_ref_inode(struct vfs_inode *inode);
void vfs_free_inode(struct vfs_inode *inode);

/* vsf提供的inode操作 */
struct vfs_stat *vfs_get_stat(struct vfs_inode *inode);
void vfs_inode_request(struct vfs_inode *inode, void *buffer, uint64_t length, uint64_t offset, uint64_t is_read);
uint64_t vfs_is_dir(struct vfs_inode *inode);
struct vfs_dir_entry *vfs_inode_dir_entry(struct vfs_inode *inode, uint64_t dir_idx);

/* vfs提供的path操作 */
struct vfs_inode *vfs_search_inode_in_dir(struct vfs_inode *inode, const char *name);
struct vfs_inode *vfs_get_inode(const char *path, struct vfs_inode *cwd);

void vfs_init();

extern struct vfs_inode *vfs_root;

#endif

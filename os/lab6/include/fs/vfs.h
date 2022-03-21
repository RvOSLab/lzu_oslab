#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <errno.h>

struct vfs_inode;
struct vfs_stat;
struct vfs_instance;
struct vfs_dir_entry;

struct vfs_interface {
    const char *fs_name;
    uint64_t fs_id;
    int64_t (*init_fs)(struct vfs_instance *fs);
    int64_t (*open_inode)(struct vfs_inode *inode);
    int64_t (*close_inode)(struct vfs_inode *inode);
    int64_t (*dir_inode)(struct vfs_inode *inode, uint64_t dir_idx, struct vfs_dir_entry *entry);
    int64_t (*inode_request)(struct vfs_inode *inode, void *buffer, uint64_t length, uint64_t offset, uint64_t is_read);
};

struct vfs_instance {
    void *fs_data;
    uint64_t ref_cnt;

    struct vfs_inode *root;
    struct vfs_interface *interface;
};

enum VFS_INODE_TYPE {
    VFS_INODE_UNKNOWN = 0,
    VFS_INODE_FILE,   // 普通文件
    VFS_INODE_DIR,    // 目录文件
    VFS_INODE_BLK,    // 块设备
    VFS_INODE_CHR,    // 字符设备
    VFS_INODE_SOCKET, // 套接字
    VFS_INODE_FIFO,   // 先入先出
    VFS_INODE_MOUNT_POINT, // 挂载点
    VFS_INODE_SYMBOL_LINK  // 符号链接
};

struct vfs_stat {
    uint32_t type;
    uint32_t gid;
    uint32_t uid;
    uint32_t nlink;
    uint64_t size;
    uint64_t atime;
    uint64_t ctime;
    uint64_t mtime;
    uint64_t inode_idx;
};

struct vfs_inode {
    void *inode_data;
    uint64_t ref_cnt;

    uint64_t inode_idx;
    struct vfs_stat stat;
    struct vfs_instance *fs;
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
struct vfs_inode *vfs_new_inode(struct vfs_instance *fs, uint64_t inode_idx);

/* vfs提供的引用计数管理操作 */
void vfs_ref_inode(struct vfs_inode *inode);
void vfs_free_inode(struct vfs_inode *inode);

/* vsf提供的inode操作 */
int64_t vfs_inode_request(struct vfs_inode *inode, void *buffer, uint64_t length, uint64_t offset, uint64_t is_read);
struct vfs_dir_entry *vfs_inode_dir_entry(struct vfs_inode *inode, uint64_t dir_idx);

/* vfs提供的path操作 */
struct vfs_inode *vfs_search_inode_in_dir(struct vfs_inode *inode, const char *name);
struct vfs_inode *vfs_get_inode(const char *path, struct vfs_inode *cwd);

void vfs_init();

extern struct vfs_inode *vfs_root;

/* 工具函数 */
static inline void vfs_instance_init(struct vfs_instance *fs, struct vfs_interface *interface) {
    fs->fs_data = NULL;
    fs->ref_cnt = 0;
    fs->root = NULL;
    fs->interface = interface;
}

#endif

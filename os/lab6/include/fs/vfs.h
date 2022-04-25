#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <errno.h>

struct vfs_inode;
struct vfs_stat;
struct vfs_instance;
struct vfs_dir_entry;

/**
 * 每个具体文件系统应提供唯一的fs_name
 * 通常的具体文件系统应且只应处理以下inode类型, 其他类型由vfs拦截转发:
 *     [VFS_INODE_FILE, VFS_INODE_DIR, VFS_INODE_SYMBOL_LINK]
 * 具体inode的引用计数应由具体文件系统维护
 * vfs_inode生命周期:
 *     [new_inode] -> open_inode -> <其他操作> x N -> flush_inode(可选<目前自动>) -> [del_inode] -> close_inode
 **/
struct vfs_interface {
    const char *fs_name;

    int64_t (*init_fs)(struct vfs_instance *fs);
    int64_t (*remove_fs)(struct vfs_instance *fs);

    int64_t (*new_inode)(struct vfs_inode *inode);   // 通过stat信息创建inode, 由具体文件系统分配inode号
    int64_t (*del_inode)(struct vfs_inode *inode);   // 通过inode号删除inode
    int64_t (*open_inode)(struct vfs_inode *inode);  // 通过inode号加载inode, 由具体文件系统提供stat
    int64_t (*flush_inode)(struct vfs_inode *inode); // 将stat信息同步到硬盘
    int64_t (*close_inode)(struct vfs_inode *inode); // 直接取消vfs_inode与真实inode的映射, 回收内存空间

    int64_t (*dir_inode)(struct vfs_inode *inode, uint64_t dir_idx, struct vfs_dir_entry *entry);            // 枚举目录inode内容
    int64_t (*add_in_dir)(struct vfs_inode *dir_inode, const char *file_name, struct vfs_inode *file_inode); // 向目录inode追加指定名称的inode
    int64_t (*del_in_dir)(struct vfs_inode *dir_inode, struct vfs_inode *file_inode);                        // 删除目录inode中的指定inode

    /* 真实操作inode内容, 忽略indoe类型 */
    int64_t (*inode_request)(struct vfs_inode *inode, void *buffer, uint64_t length, uint64_t offset, uint64_t is_read);
};

/**
 * 具体文件系统实例
 * 具体文件系统应保证init_fs成功后根inode_idx正确
 * 生命周期如下:
 *     init_fs -> <其他操作> x N -> remove_fs
 **/
struct vfs_instance {
    void *fs_data;
    uint64_t ref_cnt;

    uint64_t root_inode_idx;
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
    uint16_t type;  // enum VFS_INODE_TYPE
    uint16_t mode;  // vfs可改
    uint32_t gid;   // vfs可改
    uint32_t uid;   // vfs可改
    uint32_t nlink;
    uint64_t size;  // inode_request后必须更新
    uint64_t atime; // vfs可改
    uint64_t ctime; // vfs可改
    uint64_t mtime; // vfs可改
    uint64_t inode_idx; // vfs可改
};

/**
 * 元组(inode_idx, fs)唯一标记一个inode
 * vfs_inode的ref_cnt由vfs维护, 用于标记vfs外对该node的引用
 **/
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

/* 从指定文件系统中提取inode */
struct vfs_inode *vfs_get_inode(struct vfs_instance *fs, uint64_t inode_idx);

/* vfs提供的引用计数管理操作 */
void vfs_ref_inode(struct vfs_inode *inode);
void vfs_free_inode(struct vfs_inode *inode);

/* vsf提供的inode操作 */
int64_t vfs_inode_open(struct vfs_inode *inode, struct vfs_instance *fs, uint64_t inode_idx);
int64_t vfs_inode_close(struct vfs_inode *inode);
int64_t vfs_inode_request(struct vfs_inode *inode, void *buffer, uint64_t length, uint64_t offset, uint64_t is_read);
struct vfs_dir_entry *vfs_inode_dir_entry(struct vfs_inode *inode, uint64_t dir_idx);

/* vfs提供的path操作 */
struct vfs_inode *vfs_search_inode_in_dir(struct vfs_inode *inode, const char *name);
struct vfs_inode *vfs_get_inode_by_path(const char *path, struct vfs_inode *cwd);

void vfs_init();

extern struct vfs_instance root_fs;

/* 工具函数 */
static inline void vfs_instance_init(struct vfs_instance *fs, struct vfs_interface *interface) {
    fs->fs_data = NULL;
    fs->ref_cnt = 0;
    fs->root_inode_idx = 0;
    fs->interface = interface;
}

#endif

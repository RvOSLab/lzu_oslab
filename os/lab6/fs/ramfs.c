#include <fs/ramfs.h>

#include <kdebug.h>
#include <string.h>
#include <mm.h>

#define RAMFS_INODE_NUM (PAGE_SIZE / (sizeof(struct ramfs_inode)))

int64_t ramfs_open_inode(struct vfs_inode *inode) {
    if (inode->inode_idx >= RAMFS_INODE_NUM) return -EINVAL;
    struct ramfs_inode *inode_list = inode->fs->fs_data;
    inode->inode_data = inode_list + inode->inode_idx;
    inode->stat = (inode_list + inode->inode_idx)->stat;
    return 0;
}

int64_t ramfs_close_inode(struct vfs_inode *inode) { return 0; }

int64_t ramfs_dir_inode(struct vfs_inode *inode, uint64_t dir_idx, struct vfs_dir_entry *entry) {
    struct ramfs_inode *real_inode = (struct ramfs_inode *)inode->inode_data;
    if (dir_idx >= real_inode->length / sizeof(struct vfs_dir_entry)) return 0;
    struct vfs_dir_entry *entry_list = (struct vfs_dir_entry *)real_inode->data;
    *entry = entry_list[dir_idx];
    return 1;
}

int64_t ramfs_inode_request(struct vfs_inode *inode, void *buffer, uint64_t length, uint64_t offset, uint64_t is_read) {
    struct ramfs_inode *real_inode = (struct ramfs_inode *)inode->inode_data;
    if (offset + length > real_inode->length || offset < 0 || length < 0) return -EINVAL;
    if (is_read) {
        memcpy(buffer, real_inode->data + offset, length);
    } else {
        memcpy(real_inode->data + offset, buffer, length);
    }
    return 0;
}

void ramfs_set_inode(struct vfs_instance *fs, void *data, uint64_t length, uint64_t inode_idx, uint64_t inode_type) {
    struct ramfs_inode *real_inode = (struct ramfs_inode *)fs->fs_data;
    real_inode += inode_idx;
    real_inode->data = kmalloc(length);
    real_inode->length = length;
    real_inode->stat.type = inode_type;
    real_inode->stat.gid = 0;
    real_inode->stat.uid = 0;
    real_inode->stat.size = length;
    real_inode->stat.atime = real_inode->stat.ctime = real_inode->stat.mtime = 0;
    memcpy(real_inode->data, data, length);
}

int64_t ramfs_init_fs(struct vfs_instance *fs) {
    vfs_instance_init(fs, &ramfs_interface);
    fs->fs_data = kmalloc(PAGE_SIZE);
    if (!fs->fs_data) return -ENOMEM;
    struct ramfs_inode *inode_list = (struct ramfs_inode *)fs->fs_data;
    for (uint64_t i=0; i < RAMFS_INODE_NUM; i+=1) {
        inode_list[i].data = NULL;
        inode_list[i].length = 0;
    }
    struct vfs_dir_entry ramfs_dir[] = {
        {.name = ".",        .inode_idx = 0},
        {.name = "..",       .inode_idx = 0},
        {.name = "test.txt", .inode_idx = 1}
    };
    ramfs_set_inode(fs, ramfs_dir, sizeof(ramfs_dir), 0, RAMFS_INODE_DIR);
    const char *ramfs_test_txt = "hello ramfs\n";
    ramfs_set_inode(fs, (void *)ramfs_test_txt, strlen(ramfs_test_txt), 1, RAMFS_INODE_FILE);
    fs->root_inode_idx = 0;
    return 0;
}

struct vfs_interface ramfs_interface = {
    .fs_name = "ramfs",
    .init_fs = ramfs_init_fs,
    .open_inode = ramfs_open_inode,
    .close_inode = ramfs_close_inode,
    .dir_inode = ramfs_dir_inode,
    .inode_request = ramfs_inode_request
};

#include <fs/ramfs.h>

#include <kdebug.h>
#include <string.h>
#include <mm.h>

#define RAMFS_INODE_NUM (PAGE_SIZE / (sizeof(struct ramfs_inode)))

struct vfs_inode *ramfs_open_inode(struct vfs_inode *inode) {
    if (inode->inode_idx >= RAMFS_INODE_NUM) return NULL;
    struct ramfs_inode *inode_list = (struct ramfs_inode *)inode->fs->fs_data;
    inode->inode_data = inode_list + inode->inode_idx;
    return inode;
}

void ramfs_close_inode(struct vfs_inode *inode) {}

struct vfs_stat *ramfs_get_stat(struct vfs_inode *inode) {
    struct ramfs_inode *real_inode = (struct ramfs_inode *)inode->inode_data;
    return &real_inode->stat;
};

uint64_t ramfs_is_dir(struct vfs_inode *inode) {
    struct ramfs_inode *real_inode = (struct ramfs_inode *)inode->inode_data;
    return real_inode->type == RAMFS_INODE_DIR;
}

struct vfs_dir_entry *ramfs_dir_inode(struct vfs_inode *inode, uint64_t dir_idx) {
    struct ramfs_inode *real_inode = (struct ramfs_inode *)inode->inode_data;
    if (dir_idx >= real_inode->length) return NULL;
    struct vfs_dir_entry *entry_list = (struct vfs_dir_entry *)real_inode->data;
    return entry_list + dir_idx;
}

void ramfs_inode_request(struct vfs_inode *inode, void *buffer, uint64_t length, uint64_t offset, uint64_t is_read) {
    struct ramfs_inode *real_inode = (struct ramfs_inode *)inode->inode_data;
    if (offset + length > real_inode->length || offset < 0 || length < 0) return;
    if (is_read) {
        memcpy(buffer, real_inode->data + offset, length);
    } else {
        memcpy(real_inode->data + offset, buffer, length);
    }
}

void ramfs_set_inode(struct vfs_interface *fs, void *data, uint64_t length, uint64_t inode_idx, uint64_t inode_type) {
    struct ramfs_inode *real_inode = (struct ramfs_inode *)fs->fs_data;
    real_inode += inode_idx;
    real_inode->data = kmalloc(length);
    real_inode->length = length ;
    real_inode->type = inode_type;
    if (inode_type == RAMFS_INODE_DIR) real_inode->length /= sizeof(struct vfs_dir_entry);
    real_inode->stat.gid = 0;
    real_inode->stat.uid = 0;
    real_inode->stat.size = length;
    real_inode->stat.time = 0;
    memcpy(real_inode->data, data, length);
}

void ramfs_init_fs(struct vfs_interface *fs) {
    fs->fs_data = kmalloc(PAGE_SIZE);
    fs->ref_cnt = 0;
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
    fs->root = vfs_new_inode(fs, 0);
}

struct vfs_interface ramfs_interface = {
    .init_fs = ramfs_init_fs,
    .open_inode = ramfs_open_inode,
    .close_inode = ramfs_close_inode,
    .get_stat = ramfs_get_stat,
    .is_dir = ramfs_is_dir,
    .dir_inode = ramfs_dir_inode,
    .inode_request = ramfs_inode_request
};

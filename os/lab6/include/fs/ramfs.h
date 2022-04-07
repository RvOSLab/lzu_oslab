#ifndef RAMFS_H
#define RAMFS_H

#include <fs/vfs.h>

enum ramfs_inode_type {
    RAMFS_INODE_FILE = VFS_INODE_FILE,
    RAMFS_INODE_DIR = VFS_INODE_DIR
};

struct ramfs_inode {
    struct vfs_stat stat;
    void *data;
    uint64_t length;
};

extern struct vfs_interface ramfs_interface;

#endif

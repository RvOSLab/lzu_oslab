#ifndef RAMFS_H
#define RAMFS_H

#include <fs/vfs.h>

enum ramfs_inode_type {
    RAMFS_INODE_DIR = 0,
    RAMFS_INODE_FILE = 1
};

struct ramfs_inode {
    uint64_t type;
    struct vfs_stat stat;
    void *data;
    uint64_t length;
};

extern struct vfs_interface ramfs_interface;

#endif

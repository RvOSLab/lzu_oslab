#include <fs/vfs.h>
#include <fs/ramfs.h>

#include <assert.h>
#include <mm.h>

struct vfs_inode *vfs_root;

void vfs_init() {
    ramfs_interface.init_fs(&ramfs_interface);
    vfs_root = ramfs_interface.root;
    vfs_ref_inode(vfs_root);
}

struct vfs_inode *vfs_new_inode(struct vfs_interface *fs, uint64_t inode_idx) {
    struct vfs_inode *new_inode = kmalloc(sizeof(struct vfs_inode));
    new_inode->fs = fs;
    new_inode->fs->ref_cnt += 1;
    new_inode->inode_data = NULL;
    new_inode->inode_idx = inode_idx;
    new_inode->ref_cnt = 0;
    struct vfs_inode *opened_inode = new_inode->fs->open_inode(new_inode);
    if (!opened_inode) {
        vfs_free_inode(new_inode);
    }
    return opened_inode;
}

void vfs_ref_inode(struct vfs_inode *inode) {
    inode->ref_cnt += 1;
}

void vfs_free_inode(struct vfs_inode *inode) {
    inode->ref_cnt -= 1;
    if (!inode->ref_cnt) {
        inode->fs->close_inode(inode);
        inode->fs->ref_cnt -= 1;
        kfree(inode);
    }
}

struct vfs_stat *vfs_get_stat(struct vfs_inode *inode) {
    return inode->fs->get_stat(inode);
}

uint64_t vfs_is_dir(struct vfs_inode *inode) {
    return inode->fs->is_dir(inode);
}

void vfs_inode_request(struct vfs_inode *inode, void *buffer, uint64_t length, uint64_t offset, uint64_t is_read) {
    inode->fs->inode_request(inode, buffer, length, offset, is_read);
}

struct vfs_dir_entry *vfs_inode_dir_entry(struct vfs_inode *inode, uint64_t dir_idx) {
    return inode->fs->dir_inode(inode, dir_idx);
}

static inline int64_t vfs_pathcmp(const char *path, const char *first_name) {
    while (*path && *first_name && *path == *first_name) {
        path += 1;
        first_name += 1;
    }
    return *first_name || (*path != '/' && *path);
}

struct vfs_inode *vfs_search_inode_in_dir(struct vfs_inode *inode, const char *name) {
    uint64_t dir_idx = 0;
    while (1) {
        struct vfs_dir_entry *dir_entry = vfs_inode_dir_entry(inode, dir_idx);
        if (!dir_entry) break;
        if (!vfs_pathcmp(name, dir_entry->name)) {
            struct vfs_inode *new_inode = vfs_new_inode(inode->fs, dir_entry->inode_idx);
            return new_inode;
        }
        dir_idx += 1;
    }
    return NULL;
}

struct vfs_inode *vfs_get_inode(const char *path, struct vfs_inode *cwd) {
    struct vfs_inode *now_inode = cwd ? cwd : vfs_root;
    uint64_t char_p = 0;
    uint64_t is_first_name = 1;
    struct vfs_inode *next_inode = NULL;
    while (1) {
        uint64_t name_start_p = char_p;
        uint64_t is_last_name;
        while (path[char_p] && path[char_p] != '/') {
            char_p += 1;
        }
        is_last_name = !path[char_p];
        if (is_first_name) {
            if (name_start_p == char_p) { // path = /<...>
                now_inode = vfs_root;
            }
            is_first_name = 0;
        }
        if (name_start_p != char_p) { // path = <...>/<name>
            /* TODO: stat check */
            if (!is_last_name && !now_inode->fs->is_dir(now_inode)) return NULL;
            if (next_inode) { // previous next_inode
                next_inode = vfs_search_inode_in_dir(now_inode, path + name_start_p);
                vfs_free_inode(now_inode); // now_inode == (previous) next_inode
            } else {
                next_inode = vfs_search_inode_in_dir(now_inode, path + name_start_p);
            }
            if (!next_inode) return NULL;
            now_inode = next_inode;
        }
        if (is_last_name) {
            if (name_start_p == char_p) { // path = <...>/
                if (!now_inode->fs->is_dir(now_inode)) {
                    // (previous) next_inode != NULL == now_inode
                    if (next_inode) vfs_free_inode(now_inode);
                    return NULL;
                }
            }
            break;
        }
        char_p += 1;
    }
    return now_inode;
}

#include <fs/vfs_user.h>

int64_t vfs_user_mode_check(struct vfs_context *ctx, struct vfs_inode *inode, uint16_t mode) {
    uint32_t file_uid = inode->stat.uid;
    uint32_t file_gid = inode->stat.gid;
    uint16_t file_mode = inode->stat.mode;
    if (ctx->uid == file_uid) {
        file_mode &= S_IRWXU;
    } else if (ctx->gid == file_gid) {
        file_mode &= S_IRWXG;
    } else {
        file_mode &= S_IRWXO;
    }
    return file_mode & mode;
}

int64_t vfs_user_get_free_fd(struct vfs_context *ctx) {
    uint64_t i;
    for (i = 0; i < VFS_FD_NUM; i += 1) {
        if(!ctx->file[i]) return i;
    }
    return -EMFILE;
}

int64_t vfs_user_context_init(struct vfs_context *ctx) {
    ctx->gid = 0;
    ctx->uid = 0;
    ctx->umask = 0755;
    for (uint64_t i = 0;i < VFS_FD_NUM; i += 1) {
        ctx->file[i] = NULL;
    }
    struct vfs_inode *root_inode = vfs_get_inode(&root_fs, root_fs.root_inode_idx);
    if (!root_inode) return -EFAULT;
    ctx->cwd = root_inode;
    vfs_ref_inode(ctx->cwd);
    return 0;
}

int64_t vfs_user_context_fork(struct vfs_context *ctx, struct vfs_context *new_ctx) {
    new_ctx->gid = ctx->gid;
    new_ctx->uid = ctx->uid;
    new_ctx->umask = ctx->umask;
    for (uint64_t i = 0;i < VFS_FD_NUM; i += 1) {
        if (ctx->file[i]) {
            ctx->file[i]->ref_cnt += 1;
            new_ctx->file[i] = ctx->file[i];
        }
    }
    new_ctx->cwd = ctx->cwd;
    vfs_ref_inode(ctx->cwd);
    return 0;
}

int64_t vfs_user_open(struct vfs_context *ctx, const char *path, uint64_t flag, uint16_t mode, uint64_t fd) {
    struct vfs_file *file = (struct vfs_file *)kmalloc(sizeof(struct vfs_file));
    if (!file) return -ENOMEM;
    file->dir = NULL;
    file->position = 0;
    file->file_data = NULL;
    file->open_flag = flag;
    ctx->file[fd] = file;
    file->ref_cnt = 1;
    struct vfs_inode *inode = vfs_get_inode_by_path(path, ctx->cwd);
    if (flag & O_CREAT) {
        if ((flag & O_EXCL) && inode) {
            kfree(file);
            ctx->file[fd] = NULL;
        }
        if (!inode) {
            // create inode when set O_CREAT
        }
    } else {
        if (!inode) {
            kfree(file);
            ctx->file[fd] = NULL;
            return -ENOENT;
        }
    }
    vfs_ref_inode(inode);
    file->inode = inode;
    return 0;
}

int64_t vfs_user_close(struct vfs_context *ctx, int64_t fd) {
    if (fd < 0 || fd >= VFS_FD_NUM || !ctx->file[fd]) return -EBADF;
    struct vfs_file *file = ctx->file[fd];
    file->ref_cnt -= 1;
    if (!file->ref_cnt) {
        if (!file->inode) return -EFAULT;
        vfs_free_inode(file->inode);
        kfree(file);
        ctx->file[fd] = NULL;
    }
    return 0;
}

int64_t vfs_user_stat(struct vfs_context *ctx, int64_t fd, struct vfs_stat *stat) {
    if (fd < 0 || fd >= VFS_FD_NUM || !ctx->file[fd]) return -EBADF;
    struct vfs_file *file = ctx->file[fd];
    if (!file->inode) return -EFAULT;
    memcpy(stat, &file->inode->stat, sizeof(struct vfs_stat));
    return 0;
}

int64_t vfs_user_seek(struct vfs_context *ctx, int64_t fd, int64_t offset, int64_t whence) {
    if (fd < 0 || fd >= VFS_FD_NUM || !ctx->file[fd]) return -EBADF;
    struct vfs_file *file = ctx->file[fd];
    if (!file->inode) return -EFAULT;
    switch (whence) {
        case SEEK_SET:
            break;
        case SEEK_CUR:
            offset += file->position;
            break;
        case SEEK_END:
            offset += file->inode->stat.size;
            break;
        default:
            return -EINVAL;
    }
    if (offset < 0) return -EINVAL;
    if (offset >= file->inode->stat.size) return -EINVAL;
    file->position = offset;
    return offset;
}

int64_t vfs_user_read(struct vfs_context *ctx, int64_t fd, uint64_t length, char *buffer) {
    if (fd < 0 || fd >= VFS_FD_NUM || !ctx->file[fd]) return -EBADF;
    struct vfs_file *file = ctx->file[fd];
    if (!file->inode) return -EFAULT;
    int64_t ret = vfs_inode_request(file->inode, buffer, length, file->position, 1);
    if (ret >= 0) {
        file->position += ret;
    }
    return ret;
}

int64_t vfs_user_write(struct vfs_context *ctx, int64_t fd, uint64_t length, char *buffer) {
    if (fd < 0 || fd >= VFS_FD_NUM || !ctx->file[fd]) return -EBADF;
    struct vfs_file *file = ctx->file[fd];
    if (!file->inode) return -EFAULT;
    if (file->open_flag & O_APPEND) {
        file->position = file->inode->stat.size;
    }
    int64_t ret = vfs_inode_request(file->inode, buffer, length, file->position, 0);
    if (ret >= 0) {
        file->position += ret;
    }
    return ret;
}

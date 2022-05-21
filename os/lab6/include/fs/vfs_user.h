#ifndef VFS_USER_H
#define VFS_USER_H

#include <fs/vfs.h>
#include <mm.h>
#include <errno.h>
#include <string.h>

struct vfs_file {
    struct vfs_dir_entry *dir;
    struct vfs_inode *inode;
    uint64_t position;
    uint64_t open_flag;
    void *file_data;
    uint64_t ref_cnt;
};

#define VFS_FD_NUM 4
struct vfs_context {
    uint32_t uid;
    uint32_t gid;
    uint16_t umask;
    struct vfs_file *file[VFS_FD_NUM];
    struct vfs_inode *cwd;
};

enum vfs_user_open_flag {
    O_APPEND = 1,
    O_CREAT  = 2,
    O_EXCL   = 4
};

enum vfs_user_mode {
    S_IRWXU = 00700,
    S_IRUSR = 00400,
    S_IWUSR = 00200,
    S_IXUSR = 00100,
    S_IRWXG = 00070,
    S_IRGRP = 00040,
    S_IWGRP = 00020,
    S_IXGRP = 00010,
    S_IRWXO = 00007,
    S_IROTH = 00004,
    S_IWOTH = 00002,
    S_IXOTH = 00001,
    S_ISUID = 04000,
    S_ISGID = 02000,
    S_ISVTX = 01000
};

enum vfs_user_seek_mode {
    SEEK_SET = 1,
    SEEK_CUR = 2,
    SEEK_END = 3
};

int64_t vfs_user_open(struct vfs_context *ctx, const char *path, uint64_t flag, uint16_t mode, uint64_t fd);
int64_t vfs_user_close(struct vfs_context *ctx, int64_t fd);
int64_t vfs_user_stat(struct vfs_context *ctx, int64_t fd, struct vfs_stat *stat);
int64_t vfs_user_seek(struct vfs_context *ctx, int64_t fd, int64_t offset, int64_t whence);
int64_t vfs_user_requset(struct vfs_context *ctx, int64_t fd);
int64_t vfs_user_read(struct vfs_context *ctx, int64_t fd, uint64_t length, char *buffer);
int64_t vfs_user_write(struct vfs_context *ctx, int64_t fd, uint64_t length, char *buffer);

enum vfs_user_inode_mode {
    VFS_MODE_R = 0444,
    VFS_MODE_W = 0222,
    VFS_MODE_X = 0111
};

int64_t vfs_user_context_init(struct vfs_context *ctx);
int64_t vfs_user_context_fork(struct vfs_context *ctx, struct vfs_context *new_ctx);
int64_t vfs_user_get_free_fd(struct vfs_context *ctx);
int64_t vfs_user_mode_check(struct vfs_context *ctx, struct vfs_inode *inode, uint16_t mode);


#endif

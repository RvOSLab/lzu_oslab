#include <fs/minixfs.h>
#include <device/virtio/virtio_blk.h>
#include <device/block/block_cache.h>

// static minixfs_get_bitmap();

int64_t minixfs_init_fs(struct vfs_instance *fs) {
    struct minixfs_context *ctx = (struct minixfs_context *)kmalloc(sizeof(struct minixfs_context));
    if (!ctx) return -ENOMEM;
    ctx->dev = get_dev_by_major_minor(VIRTIO_MAJOR, 2);
    if (!ctx->dev)  return -ENODEV;
    if (!(ctx->dev->interface_flag & BLOCK_INTERFACE_BIT)) return -ENOTBLK;

    struct block_cache_request req = {
        .request_flag = BLOCK_READ,
        .length = sizeof(struct minixfs_super_block),
        .offset = MINIXFS_ZONE_SIZE,
        .target = &ctx->super_block
    };
    int64_t ret = block_cache_request(ctx->dev, &req);
    if (ret < 0) return -EIO;

    if (ctx->super_block.magic != 0x137f) return -EINVAL;

    fs->fs_data = ctx;
    return 0;
}

struct vfs_interface minixfs_interface = {
    .fs_name = "minixfs",
    .init_fs = minixfs_init_fs,
    .open_inode = NULL,
    .close_inode = NULL,
    .dir_inode = NULL,
    .inode_request = NULL
};

#include <fs/minixfs.h>
#include <device/virtio/virtio_blk.h>
#include <device/block/block_cache.h>
#include <kdebug.h>
#include <string.h>

static int64_t minixfs_init_fs(struct vfs_instance *fs) {
    struct device *dev = get_dev_by_major_minor(VIRTIO_MAJOR, 2);
    if (!dev)  return -ENODEV;
    if (!(dev->interface_flag & BLOCK_INTERFACE_BIT)) return -ENOTBLK;

    struct minixfs_context *ctx = (struct minixfs_context *)kmalloc(sizeof(struct minixfs_context));
    if (!ctx) return -ENOMEM;
    ctx->dev = dev;
    struct block_cache_request req = {
        .request_flag = BLOCK_READ,
        .length = sizeof(struct minixfs_super_block),
        .offset = MINIXFS_ZONE_SIZE,
        .target = &ctx->super_block
    };
    int64_t ret = block_cache_request(ctx->dev, &req);
    if (ret < 0) {
        kfree(ctx);
        return ret;
    }

    if (ctx->super_block.magic != 0x137f) {
        kfree(ctx);
        return -EINVAL;
    }
    ctx->block_size = MINIXFS_ZONE_SIZE << ctx->super_block.log_zone_size;
    uint64_t inode_map_length = ctx->block_size * ctx->super_block.imap_blk;
    ctx->inode_bitmap_offset = MINIXFS_ZONE_SIZE * 2;
    uint64_t zone_map_length = ctx->block_size * ctx->super_block.zmap_blk;
    ctx->zone_bitmap_offset = ctx->inode_bitmap_offset + inode_map_length;
    ctx->inode_offset = ctx->zone_bitmap_offset + zone_map_length;
    ctx->zone_offset = ctx->block_size * ctx->super_block.fst_data_zone;

    fs->fs_data = ctx;
    fs->root_inode_idx = 1;
    return 0;
}

static void minixfs_to_vfs_stat(struct minixfs_inode *m_inode, struct vfs_stat *v_stat) {
    v_stat->type = (m_inode->mode & MINIXFS_IFLNK) == MINIXFS_IFLNK ? VFS_INODE_SYMBOL_LINK
                 : (m_inode->mode & MINIXFS_IFREG) == MINIXFS_IFREG ? VFS_INODE_FILE
                 : (m_inode->mode & MINIXFS_IFDIR) == MINIXFS_IFDIR ? VFS_INODE_DIR
                 : VFS_INODE_UNKNOWN;
    v_stat->mode = m_inode->mode & MINIXFS_MODE_MASK;
    v_stat->gid = m_inode->gid;
    v_stat->uid = m_inode->uid;
    v_stat->nlink = m_inode->nlinks;
    v_stat->size = m_inode->size;
    v_stat->atime = m_inode->time;
    v_stat->ctime = m_inode->time;
    v_stat->mtime = m_inode->time;
}

static int64_t minixfs_close_inode(struct vfs_inode *inode) {
    struct minixfs_inode *real_inode = (struct vfs_inode *)inode->inode_data;
    kfree(real_inode);
}

static int64_t minixfs_imap_get(struct minixfs_context *ctx, uint64_t idx) {
    uint8_t bitmap;
    if (!idx) return -EINVAL;
    idx -= 1;
    if ((idx >> 3) >= ctx->zone_bitmap_offset) return -EINVAL;
    struct block_cache_request req = {
        .request_flag = BLOCK_READ,
        .length = sizeof(uint8_t),
        .offset = ctx->inode_bitmap_offset + (idx >> 3),
        .target = &bitmap
    };
    int64_t ret = block_cache_request(ctx->dev, &req);
    if (ret < 0) return ret;
    return bitmap & (1 << (idx & 7));
}

static int64_t minixfs_imap_set(struct minixfs_context *ctx, uint64_t idx) {
    uint8_t bitmap;
    if (!idx) return -EINVAL;
    idx -= 1;
    if ((idx >> 3) >= ctx->zone_bitmap_offset) return -EINVAL;
    struct block_cache_request req = {
        .request_flag = BLOCK_READ,
        .length = sizeof(uint8_t),
        .offset = ctx->inode_bitmap_offset + (idx >> 3),
        .target = &bitmap
    };
    int64_t ret = block_cache_request(ctx->dev, &req);
    if (ret < 0) return ret;
    bitmap |= (1 << (idx & 7));
    req.request_flag = BLOCK_WRITE;
    block_cache_request(ctx->dev, &req);
    if (ret < 0) return ret;
    return 0;
}

static int64_t minixfs_zmap_get(struct minixfs_context *ctx, uint64_t idx) {
    uint8_t bitmap;
    if (!idx) return -EINVAL;
    idx -= 1;
    if ((idx >> 3) >= ctx->inode_offset) return -EINVAL;
    struct block_cache_request req = {
        .request_flag = BLOCK_READ,
        .length = sizeof(uint8_t),
        .offset = ctx->zone_bitmap_offset + (idx >> 3),
        .target = &bitmap
    };
    int64_t ret = block_cache_request(ctx->dev, &req);
    if (ret < 0) return ret;
    return bitmap & (1 << (idx & 7));
}

static int64_t minixfs_zmap_set(struct minixfs_context *ctx, uint64_t idx) {
    uint8_t bitmap;
    if (!idx) return -EINVAL;
    idx -= 1;
    if ((idx >> 3) >= ctx->inode_offset) return -EINVAL;
    idx -= 1;
    struct block_cache_request req = {
        .request_flag = BLOCK_READ,
        .length = sizeof(uint8_t),
        .offset = ctx->zone_bitmap_offset + (idx >> 3),
        .target = &bitmap
    };
    int64_t ret = block_cache_request(ctx->dev, &req);
    if (ret < 0) return ret;
    bitmap |= (1 << (idx & 7));
    req.request_flag = BLOCK_WRITE;
    block_cache_request(ctx->dev, &req);
    if (ret < 0) return ret;
    return 0;
}

static int64_t minixfs_open_inode(struct vfs_inode *inode) {
    struct minixfs_context *ctx = (struct minixfs_context *)inode->fs->fs_data;
    if (minixfs_imap_get(ctx, inode->inode_idx) <= 0) return -EINVAL;
    struct minixfs_inode *real_inode = (struct minixfs_inode *)kmalloc(sizeof(struct minixfs_inode));
    if (!real_inode) return -ENOMEM;
    struct block_cache_request req = {
        .request_flag = BLOCK_READ,
        .length = sizeof(struct minixfs_inode),
        .offset = ctx->inode_offset + (inode->inode_idx - 1) * sizeof(struct minixfs_inode),
        .target = real_inode
    };
    int64_t ret = block_cache_request(ctx->dev, &req);
    if (ret < 0) {
        kfree(real_inode);
        return ret;
    }
    inode->inode_data = real_inode;
    minixfs_to_vfs_stat(real_inode, &inode->stat);
    inode->stat.inode_idx = inode->inode_idx;
    return 0;
}

int64_t minixfs_zone_walk(struct vfs_inode *inode) {
    struct minixfs_context *ctx = (struct minixfs_context *)inode->fs->fs_data;
    struct minixfs_inode *real_inode = (struct minixfs_inode *)inode->inode_data;

    /* 一级zone (zone_level  < 7) */
    for (uint64_t zone_level = 0; zone_level < 7; zone_level += 1) {
        uint16_t zone_idx = real_inode->zone[zone_level];
        if (minixfs_zmap_get(ctx, zone_idx) <= 0) return -EINVAL;
        // done
    }

    uint64_t sub_zone_idx_num = ctx->block_size / sizeof(uint16_t);

    /* 二级zone (zone_level == 7)*/
    uint16_t zone_idx_1 = real_inode->zone[7];
    if (minixfs_zmap_get(ctx, zone_idx_1) <= 0) return -EINVAL;
    for (uint64_t second_idx = 0; second_idx < sub_zone_idx_num; second_idx += 1) {
        uint16_t zone_idx;
        struct block_cache_request req = {
            .request_flag = BLOCK_READ,
            .length = sizeof(uint16_t),
            .offset = zone_idx_1 * ctx->block_size + second_idx * sizeof(uint16_t),
            .target = &zone_idx
        };
        int ret = block_cache_request(ctx->dev, &req);
        if (ret < 0) return ret;
        if (minixfs_zmap_get(ctx, zone_idx) <= 0) return -EINVAL;
        // done
    }

    /* 三级zone (zone_level == 8) */
    zone_idx_1 = real_inode->zone[8];
    if (minixfs_zmap_get(ctx, zone_idx_1) <= 0) return -EINVAL;
    uint64_t sub_zone_idx_num = ctx->block_size / sizeof(uint16_t);
    for (uint64_t second_idx = 0; second_idx < sub_zone_idx_num; second_idx += 1) {
        uint16_t zone_idx_2;
        struct block_cache_request req = {
            .request_flag = BLOCK_READ,
            .length = sizeof(uint16_t),
            .offset = zone_idx_1 * ctx->block_size + second_idx * sizeof(uint16_t),
            .target = &zone_idx_2
        };
        int ret = block_cache_request(ctx->dev, &req);
        if (ret < 0) return ret;
        if (minixfs_zmap_get(ctx, zone_idx_2) <= 0) return -EINVAL;
        for (uint64_t third_idx = 0; third_idx < sub_zone_idx_num; third_idx += 1) {
            uint16_t zone_idx;
            struct block_cache_request req = {
                .request_flag = BLOCK_READ,
                .length = sizeof(uint16_t),
                .offset = zone_idx_2 * ctx->block_size + third_idx * sizeof(uint16_t),
                .target = &zone_idx
            };
            int ret = block_cache_request(ctx->dev, &req);
            if (ret < 0) return ret;
            if (minixfs_zmap_get(ctx, zone_idx) <= 0) return -EINVAL;
            // done
        }
    }
}

static int64_t minixfs_inode_request(struct vfs_inode *inode, void *buffer, uint64_t length, uint64_t offset, uint64_t is_read) {
    struct minixfs_context *ctx = (struct minixfs_context *)inode->fs->fs_data;
    struct minixfs_inode *real_inode = (struct minixfs_inode *)inode->inode_data;
    uint64_t bytes_counter = 0;
    uint64_t zone_level;
    // TODO: support write
    for (zone_level = 0; zone_level < 7; zone_level += 1) {
        // TODO: check zone map
        uint64_t zone_idx = real_inode->zone[zone_level];
        uint64_t real_read_length = ctx->block_size;
        if (offset < real_read_length) {
            real_read_length -= offset;
            if ((length - bytes_counter) < real_read_length) {
                real_read_length = length - bytes_counter;
            }
            struct block_cache_request req = {
                .request_flag = is_read ? BLOCK_READ : BLOCK_WRITE,
                .length = real_read_length,
                .offset = zone_idx * ctx->block_size + offset,
                .target = buffer + bytes_counter
            };
            if (req.offset < ctx->zone_offset) {
                return -EAGAIN;
            }
            int64_t ret = block_cache_request(ctx->dev, &req);
            if (ret < 0) return ret;
            bytes_counter += real_read_length;
            offset = 0;
        } else {
            offset -= real_read_length;
        }
        if (bytes_counter == length) break;
    }
    // TODO: support huge zone_level
    if (zone_level == 7) kputs("warn: huge zone_level");
    return bytes_counter;
}

static int64_t minixfs_dir_inode(struct vfs_inode *inode, uint64_t dir_idx, struct vfs_dir_entry *entry) {
    struct minixfs_context *ctx = (struct minixfs_context *)inode->fs->fs_data;
    struct minixfs_inode *real_inode = (struct minixfs_inode *)inode->inode_data;
    uint64_t real_entry_len = 16;
    if (dir_idx > (real_inode->size / real_entry_len)) return 0;
    struct minixfs_dir_entry *dir_entry = kmalloc(real_entry_len);
    int64_t ret = minixfs_inode_request(inode, dir_entry, real_entry_len, real_entry_len * dir_idx, 1);
    if (ret < 0) return ret;
    if (ret != real_entry_len) return -EIO;
    entry->inode_idx = dir_entry->inode;
    entry->name = kmalloc(strlen(dir_entry->name));
    if (!entry->name) return -ENOMEM;
    memcpy(entry->name, dir_entry->name, strlen(dir_entry->name));
    // TODO: kfree name str
    return 1;
}

struct vfs_interface minixfs_interface = {
    .fs_name = "minixfs",
    .init_fs = minixfs_init_fs,
    .open_inode = minixfs_open_inode,
    .close_inode = minixfs_close_inode,
    .dir_inode = minixfs_dir_inode,
    .inode_request = minixfs_inode_request
};

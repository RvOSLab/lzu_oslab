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
    struct minixfs_inode *real_inode = (struct minixfs_inode *)inode->inode_data;
    kfree(real_inode);
    return 0;
}

static int64_t minixfs_imap_get(struct minixfs_context *ctx, uint64_t idx) {
    uint8_t bitmap;
    if (!idx || (idx >> 3) >= ctx->zone_bitmap_offset) return -EINVAL;
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
    if (!idx || (idx >> 3) >= ctx->zone_bitmap_offset) return -EINVAL;
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
    if (idx < ctx->super_block.fst_data_zone) return -EINVAL;
    idx -= ctx->super_block.fst_data_zone;
    if (idx >= ctx->super_block.nzones) return -EINVAL;
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
    if (idx < ctx->super_block.fst_data_zone) return -EINVAL;
    idx -= ctx->super_block.fst_data_zone;
    if (idx >= ctx->super_block.nzones) return -EINVAL;
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

static int64_t minixfs_new_inode(struct vfs_inode *inode) {
    struct minixfs_context *ctx = (struct minixfs_context *)inode->fs->fs_data;
    uint16_t idx = 1;
    while (1) {
        int64_t map_bit = minixfs_imap_get(ctx, idx);
        if (map_bit < 0) return map_bit;
        if (!map_bit) {
            inode->inode_idx = idx;
            minixfs_imap_set(ctx, idx);
            return 0;
        }
        idx += 1;
    }
}

static int64_t minixfs_del_inode(struct vfs_inode *inode) {
    struct minixfs_context *ctx = (struct minixfs_context *)inode->fs->fs_data;
    int64_t ret = minixfs_imap_set(ctx, inode->inode_idx);
    return ret;
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

static uint16_t minixfs_zone_new(struct minixfs_context *ctx) {
    uint16_t idx = ctx->super_block.fst_data_zone;
    while (1) {
        int64_t map_bit = minixfs_zmap_get(ctx, idx);
        if (map_bit < 0) return 0;
        if (!map_bit) {
            minixfs_zmap_set(ctx, idx);
            char buffer[ctx->block_size];
            memset(buffer, 0, ctx->block_size);
            struct block_cache_request req = { // zone填0
                .request_flag = BLOCK_WRITE,
                .length = ctx->block_size,
                .offset = idx * ctx->block_size,
                .target = buffer
            };
            int64_t ret = block_cache_request(ctx->dev, &req);
            if (ret < 0) return 0;
            return idx;
        }
        idx += 1;
    }
}

static int64_t minixfs_flush_inode(struct vfs_inode *inode) {
    struct minixfs_context *ctx = (struct minixfs_context *)inode->fs->fs_data;
    struct minixfs_inode *real_inode = (struct minixfs_inode *)inode->inode_data;
    struct block_cache_request req = { // 回写inode
        .request_flag = BLOCK_WRITE,
        .length = sizeof(struct minixfs_inode),
        .offset = ctx->inode_offset +
                  (inode->inode_idx - 1) * sizeof(struct minixfs_inode),
        .target = real_inode
    };
    return block_cache_request(ctx->dev, &req);
}

static int64_t minixfs_inode_request(struct vfs_inode *inode, void *buffer, uint64_t length, uint64_t offset, uint64_t is_read) {
    struct minixfs_context *ctx = (struct minixfs_context *)inode->fs->fs_data;
    struct minixfs_inode *real_inode = (struct minixfs_inode *)inode->inode_data;

    /* 拷贝计数器 */
    uint64_t request_end = length + offset;
    uint64_t bytes_counter = 0;
    uint64_t real_read_length;

    /* 一级zone (zone_level  < 7) */
    for (uint64_t zone_level = 0; zone_level < 7; zone_level += 1) {
        uint64_t zone_idx = real_inode->zone[zone_level];
        if (!zone_idx) { // 未分配zone处理
            if (is_read) return bytes_counter;
            zone_idx = minixfs_zone_new(ctx);
            if (!zone_idx) return -EIO;
            real_inode->zone[zone_level] = zone_idx;
            int ret = minixfs_flush_inode(inode);
            if (ret < 0) return ret;
        }
        real_read_length = ctx->block_size;
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
        if (bytes_counter == length) {
            if (!is_read) {
                if (request_end >= real_inode->size) {
                        real_inode->size = request_end;
                        inode->stat.size = request_end;
                        int64_t ret = minixfs_flush_inode(inode);
                        if (ret < 0) return ret;
                }
            }
            return bytes_counter;
        }
    }

    uint64_t sub_zone_idx_num = ctx->block_size / sizeof(uint16_t);

    /* 二级zone (zone_level == 7)*/
    uint16_t zone_idx_1 = real_inode->zone[7];
    if (!zone_idx_1) { // 未分配zone处理
        if (is_read) return bytes_counter;
        zone_idx_1 = minixfs_zone_new(ctx);
        if (!zone_idx_1) return -EIO;
        real_inode->zone[7] = zone_idx_1;
        int ret = minixfs_flush_inode(inode);
        if (ret < 0) return ret;
    }
    for (uint64_t second_idx = 0; second_idx < sub_zone_idx_num; second_idx += 1) {
        uint16_t zone_idx;
        struct block_cache_request req = { // 读取zone号
            .request_flag = BLOCK_READ,
            .length = sizeof(uint16_t),
            .offset = zone_idx_1 * ctx->block_size + second_idx * sizeof(uint16_t),
            .target = &zone_idx
        };
        int ret = block_cache_request(ctx->dev, &req);
        if (ret < 0) return ret;
        if (!zone_idx) { // 未分配zone处理
            if (is_read) return bytes_counter;
            zone_idx = minixfs_zone_new(ctx);
            if (!zone_idx) return -EIO;
            req.request_flag = BLOCK_WRITE;
            ret = block_cache_request(ctx->dev, &req);
            if (ret < 0) return ret;
            ret = minixfs_flush_inode(inode);
            if (ret < 0) return ret;
        }
        real_read_length = ctx->block_size;
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
        if (bytes_counter == length) {
            if (!is_read) {
                if (request_end >= real_inode->size) {
                        real_inode->size = request_end;
                        inode->stat.size = request_end;
                        int64_t ret = minixfs_flush_inode(inode);
                        if (ret < 0) return ret;
                }
            }
            return bytes_counter;
        }
    }

    /* 三级zone (zone_level == 8) */
    zone_idx_1 = real_inode->zone[8];
    if (!zone_idx_1) { // 未分配zone处理
        if (is_read) return bytes_counter;
        zone_idx_1 = minixfs_zone_new(ctx);
        if (!zone_idx_1) return -EIO;
        real_inode->zone[8] = zone_idx_1;
        int ret = minixfs_flush_inode(inode);
        if (ret < 0) return ret;
    }
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
        if (!zone_idx_2) { // 未分配zone处理
            if (is_read) return bytes_counter;
            zone_idx_2 = minixfs_zone_new(ctx);
            if (!zone_idx_2) return -EIO;
            req.request_flag = BLOCK_WRITE;
            ret = block_cache_request(ctx->dev, &req);
            if (ret < 0) return ret;
        }
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
            if (!zone_idx) { // 未分配zone处理
                if (is_read) return bytes_counter;
                zone_idx = minixfs_zone_new(ctx);
                if (!zone_idx) return -EIO;
                req.request_flag = BLOCK_WRITE;
                ret = block_cache_request(ctx->dev, &req);
                if (ret < 0) return ret;
                ret = minixfs_flush_inode(inode);
                if (ret < 0) return ret;
            }
            real_read_length = ctx->block_size;
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
            if (bytes_counter == length) {
                if (!is_read) {
                    if (request_end >= real_inode->size) {
                        real_inode->size = request_end;
                        inode->stat.size = request_end;
                        int64_t ret = minixfs_flush_inode(inode);
                        if (ret < 0) return ret;
                    }
                }
                return bytes_counter;
            }
        }
    }
    if (!is_read) {
        if (request_end >= real_inode->size) {
                real_inode->size = request_end;
                inode->stat.size = request_end;
                int64_t ret = minixfs_flush_inode(inode);
                if (ret < 0) return ret;
        }
    }
    return bytes_counter;
}

static int64_t minixfs_dir_inode(struct vfs_inode *inode, uint64_t dir_idx, struct vfs_dir_entry *entry) {
    struct minixfs_inode *real_inode = (struct minixfs_inode *)inode->inode_data;
    uint64_t real_entry_len = sizeof(struct minixfs_dir_entry);
    if (dir_idx >= (real_inode->size / real_entry_len)) return 0;
    struct minixfs_dir_entry dir_entry;
    int64_t ret = minixfs_inode_request(inode, &dir_entry, real_entry_len, real_entry_len * dir_idx, 1);
    if (ret < 0) return ret;
    if (ret != real_entry_len) return -EIO;
    entry->inode_idx = dir_entry.inode;
    uint64_t str_len = strlen(dir_entry.name);
    if (str_len > 14) str_len = 14;
    entry->name = kmalloc(str_len + 1);
    if (!entry->name) return -ENOMEM;
    memcpy((char *)entry->name, dir_entry.name, str_len + 1);
    return 1;
}

static int64_t minixfs_add_in_dir(struct vfs_inode *dir_inode, const char *file_name, struct vfs_inode *file_inode) {
    struct minixfs_inode *real_inode = (struct minixfs_inode *)dir_inode->inode_data;
    uint64_t real_entry_len = sizeof(struct minixfs_dir_entry);
    uint64_t str_len = strlen(file_name);
    if (str_len > 14) return -EFBIG;
    struct minixfs_dir_entry dir_entry = {
        .inode = file_inode->inode_idx
    };
    memcpy(dir_entry.name, file_name, str_len);
    memset(dir_entry.name, 0, 14 - str_len);
    int64_t ret = minixfs_inode_request(dir_inode, &dir_entry, real_entry_len, real_inode->size, 0);
    if (ret < 0) return ret;
    return 0;
}

static int64_t minixfs_del_in_dir(struct vfs_inode *dir_inode, struct vfs_inode *file_inode) {
    struct minixfs_inode *real_inode = (struct minixfs_inode *)dir_inode->inode_data;
    uint64_t real_entry_len = 16;
    uint64_t dir_idx = 0;
    struct minixfs_dir_entry dir_entry;
    while (dir_idx < (real_inode->size / real_entry_len)) {
        int64_t ret = minixfs_inode_request(dir_inode, &dir_entry, real_entry_len, real_entry_len * dir_idx, 1);
        if (ret < 0) return ret;
        if (ret != real_entry_len) return -EIO;
        if (dir_entry.inode == file_inode->inode_idx) {
            memset(&dir_entry, 0, real_entry_len);
            ret = minixfs_inode_request(dir_inode, &dir_entry, real_entry_len, real_entry_len * dir_idx, 0);
            if (ret < 0) return ret;
            return 0;
        }
        dir_idx += 1;
    }
    return -ENOENT;
}

struct vfs_interface minixfs_interface = {
    .fs_name = "minixfs",
    .init_fs = minixfs_init_fs,

    .new_inode = minixfs_new_inode,
    .del_inode = minixfs_del_inode,
    .open_inode = minixfs_open_inode,
    .flush_inode = minixfs_flush_inode,
    .close_inode = minixfs_close_inode,

    .dir_inode = minixfs_dir_inode,
    .add_in_dir = minixfs_add_in_dir,
    .del_in_dir = minixfs_del_in_dir,

    .inode_request = minixfs_inode_request
};

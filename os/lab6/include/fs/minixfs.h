#ifndef MINIXFS_H
#define MINIXFS_H

#include <fs/vfs.h>

#define MINIXFS_ZONE_SIZE 1024

struct minixfs_super_block {
    uint16_t ninodes;       // 文件系统含有的 inode 数量, 用于计算 Inode-Bitmap 和 Inode-Table 的大小
    uint16_t nzones;        // 文件系统中含有的 Zone 的数量, 该值用于计算 Zone-Bitmap
    uint16_t imap_blk;      // 描述 Inode-BitMap 占用 BLOCK_SIZE 的数量
    uint16_t zmap_blk;      // 描述 Zone-BitMap 占用 BLOCK_SIZE 的数量
    uint16_t fst_data_zone; // 描述第一个 Data Zone 所在的 block 号
    uint16_t log_zone_size; // 一个虚拟块的大小; BLOCK_SIZE = 1024 << log_zone_size

    uint32_t max_size;      // 能存放的最大文件大小(以 byte 计数)
    uint16_t magic;         // 魔数(v1 0x137f; v2 0x138f)
    uint16_t state;         // mount 状态
};

enum MINIXFS_INODE_TYPE {
    MINIXFS_IFLNK = 0120000,
    MINIXFS_IFREG = 0100000,
    MINIXFS_IFDIR = 0040000
};
#define MINIXFS_MODE_MASK 07777

struct minixfs_inode {
    uint16_t mode;  // 类型和访问属性
    uint16_t uid;   // uid
    uint32_t size;  // inode 的大小
    uint32_t time;  // 修改 inode 的最后时间
    uint8_t gid;    // gid
    uint8_t nlinks; // inode 被 link 的数量
    uint16_t zone[9]; // 描述一个 inode 所有内容的 zone 号
};

struct minixfs_dir_entry {
    uint16_t inode; // inode
    char name[14];   // 文件/目录名
};

struct minixfs_context {
    struct minixfs_super_block super_block;
    struct device *dev;
    uint64_t block_size;
    uint64_t inode_bitmap_offset;
    uint64_t zone_bitmap_offset;
    uint64_t inode_offset;
    uint64_t zone_offset;
};

extern struct vfs_interface minixfs_interface;

#endif

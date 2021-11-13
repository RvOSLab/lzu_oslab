#include <mm.h>
#include <stddef.h>

static const uint8_t debruijn[64] = {
    63,  0, 58,  1, 59, 47, 53,  2,
    60, 39, 48, 27, 54, 33, 42,  3,
    61, 51, 37, 40, 49, 18, 28, 20,
    55, 30, 34, 11, 43, 14, 22,  4,
    62, 57, 46, 52, 38, 26, 32, 41,
    50, 36, 17, 19, 29, 10, 13, 21,
    56, 45, 25, 31, 35, 16,  9, 12,
    44, 24, 15,  8, 23,  7,  6,  5
};

static inline uint8_t q_log2_ceil(uint64_t n) {
    n -= 1;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return debruijn[((uint64_t)((n + 1) * 0x07EDD5E59A4E28C2)) >> 58];
}

static inline uint8_t q_pow2_factor(uint64_t n) {
    return debruijn[((uint64_t)((n & -n) * 0x07EDD5E59A4E28C2)) >> 58];
}

/* 可分配的块大小：16B - 4KB(一整页) */
#define PAGE_SIZE_LOG2 12
#define MIN_ALLOC_SIZE_LOG2 4
#define MAX_ALLOC_SIZE_LOG2 PAGE_SIZE_LOG2

/* [16, 32, 64, 128, 256, 512, 1024, 2048, 4096] */
struct bucket_desc* bucket_dir[MAX_ALLOC_SIZE_LOG2 - MIN_ALLOC_SIZE_LOG2 + 1] = { NULL };

/* 存储桶描述符结构，32 Bytes */
struct bucket_desc {
    struct bucket_desc *next; /* 下一个桶描述符 */

    uint64_t page;    /* 存放块的内存页面 */
    uint64_t freeidx; /* 本桶中第一个空闲块的索引 */
    uint64_t refcnt;  /* 已分配块计数 */
};

struct bucket_desc* add_some_bucket_desc(uint8_t idx) {
    uint64_t new_page = VIRTUAL(get_free_page());
    uint64_t bucket_addr = new_page;
    struct bucket_desc *bucket;
    while (bucket_addr < new_page + PAGE_SIZE) {
        bucket = (struct bucket_desc *) bucket_addr;
        bucket->page = 0;
        bucket->next = bucket + 1;
        bucket_addr = (uint64_t) bucket->next;
    }
    bucket->next = bucket_dir[idx];
    bucket_dir[idx] = (struct bucket_desc *) new_page;
    return bucket_dir[idx];
}

void init_bucket_desc(struct bucket_desc * bucket, uint64_t alloc_size) {
    uint64_t new_page = VIRTUAL(get_free_page());
    uint8_t *idx_ptr;
    uint64_t block_count = 1 << (PAGE_SIZE_LOG2 - alloc_size);
    for (uint64_t i = 0; i < block_count; i += 1) {
        idx_ptr = (uint8_t *)(new_page + (i << alloc_size));
        *idx_ptr = i + 1;
    }
    bucket->page = new_page;
    bucket->refcnt = 0;
    bucket->freeidx = 0;
}

void* kmalloc(uint64_t size) {
    /* 计算实际分配的块大小 */
    uint8_t alloc_size = q_log2_ceil(size); /* 指数形式 */
    switch (alloc_size) {
        case 0 ... (MIN_ALLOC_SIZE_LOG2 - 1):
            alloc_size = MIN_ALLOC_SIZE_LOG2;
        case MIN_ALLOC_SIZE_LOG2 ... MAX_ALLOC_SIZE_LOG2:
            break;
        default: /* 大于4KB或为0 */
            return NULL;
    }
    /* 找到对应块大小的未满的桶 */
    struct bucket_desc* bucket = bucket_dir[alloc_size - MIN_ALLOC_SIZE_LOG2];
    while (bucket) {
        if (bucket->refcnt >> (PAGE_SIZE_LOG2 - alloc_size)) { /* 桶已满 */
            bucket = bucket->next;
            continue;
        }
        break;
    }
    if (!bucket) bucket = add_some_bucket_desc(alloc_size - MIN_ALLOC_SIZE_LOG2);
    if (!bucket->page) init_bucket_desc(bucket, alloc_size);
    /* 从桶中获取一个空闲块 */
    bucket->refcnt += 1;
    uint64_t free_block = bucket->page + (bucket->freeidx << alloc_size);
    bucket->freeidx = *((uint8_t *) free_block);
    return (void *) free_block;
}

void kfree_s(void* ptr, uint64_t size) {
    uint64_t addr = (uint64_t) ptr;
    /* 推算分配的块大小范围 */
    uint8_t alloc_size_start = MIN_ALLOC_SIZE_LOG2;
    uint8_t alloc_size_end = MAX_ALLOC_SIZE_LOG2;
    if (size) {
        uint8_t alloc_size = q_log2_ceil(size);
        switch (alloc_size) {
            case 0 ... (MIN_ALLOC_SIZE_LOG2 - 1):
                alloc_size = MIN_ALLOC_SIZE_LOG2;
            case MIN_ALLOC_SIZE_LOG2 ... MAX_ALLOC_SIZE_LOG2:
                alloc_size_start = alloc_size_end = alloc_size;
                break;
            default: /* 大于4KB */
                return;
        }
    } else {
        uint8_t aligned_size = q_pow2_factor(addr);
        if (aligned_size < MIN_ALLOC_SIZE_LOG2) return;
        if (aligned_size < MAX_ALLOC_SIZE_LOG2) alloc_size_end = aligned_size;
    }
    /* 找到该块所在的桶 */
    struct bucket_desc* bucket;
    uint64_t page_addr = (addr >> PAGE_SIZE_LOG2) << PAGE_SIZE_LOG2;
    uint8_t guess_alloc_size = alloc_size_start;
    while (guess_alloc_size <= alloc_size_end) {
        bucket = bucket_dir[guess_alloc_size - MIN_ALLOC_SIZE_LOG2];
        while (bucket) {
            if (bucket->page == page_addr) break;
            bucket = bucket->next;
        }
        if (bucket) break;
        guess_alloc_size += 1;
    }
    if(!bucket) return;
    /* 将该块放回桶中 */
    bucket->refcnt -= 1;
    if(bucket->refcnt) {
        *((uint8_t *) ptr) = bucket->freeidx;
        bucket->freeidx = (addr - page_addr) >> guess_alloc_size;
    } else {
        bucket->page = 0;
        free_page(PHYSICAL(page_addr));
    }
}
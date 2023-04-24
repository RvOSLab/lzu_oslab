#ifdef COMPILE
/**
 * @file malloc.c
 * @brief 实现内核内存管理
 *
 * 本模块实现了kmalloc与kfree_s函数，从而允许内核程序动态的申请和释放内存。
 */

#include <mm.h>
#include <stddef.h>

/* for malloc_test() */
#include <assert.h>
#include <kdebug.h>

/* DeBruijn序列 */
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

/**
 * @brief 快速log2(向上取整)
 *
 * 计算下一个大于n的2的幂次(向上取整)，如n = 9 <= 2 ** 4，返回4。
 *
 * @param n   64位无符号整数
 * @return ceil(log2(n))
 * @note n = 0 时返回 63
 */
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

/**
 * @brief 快速计算素因子中所含2的个数
 *
 * 设n = (2*k+1) * 2 ** p，返回p。
 *
 * @param n   64位无符号整数
 * @return 素因子中所含2的个数
 */
static inline uint8_t q_pow2_factor(uint64_t n) {
    return debruijn[((uint64_t)((n & -n) * 0x07EDD5E59A4E28C2)) >> 58];
}

/* 可分配的块大小：16B - 4KB(一整页) */
#define PAGE_SIZE_LOG2 12
#define MIN_ALLOC_SIZE_LOG2 4
#define MAX_ALLOC_SIZE_LOG2 PAGE_SIZE_LOG2
/* 特殊桶大小 */
#define SPECIAL_BUCKET_SIZE_LOG2 5

/* 桶描述符目录 [16, 32, 64, 128, 256, 512, 1024, 2048, 4096] */
struct bucket_desc* bucket_dir[MAX_ALLOC_SIZE_LOG2 - MIN_ALLOC_SIZE_LOG2 + 1] = { NULL };

/* 存储桶描述符结构，32 Bytes */
struct bucket_desc {
    struct bucket_desc *next; /* 下一个桶描述符 */

    uint64_t page;    /* 存放块的内存页面 */
    uint64_t freeidx; /* 本桶中第一个空闲块的索引 */
    uint64_t refcnt;  /* 已分配块计数 */
};

/**
 * @brief 初始化指定的桶页面
 *
 * 将指定的桶页面按给定的块大小初始化。
 *
 * @param page_addr 桶页面地址
 * @param alloc_size 分配的块大小(指数形式)
 */
void init_bucket_page(uint64_t page_addr, uint8_t alloc_size) {
    uint8_t *idx_ptr;
    uint64_t block_count = 1 << (PAGE_SIZE_LOG2 - alloc_size);
    for (uint64_t i = 0; i < block_count; i += 1) {
        idx_ptr = (uint8_t *)(page_addr + (i << alloc_size));
        *idx_ptr = i + 1;
    }
}

/**
 * @brief 取得空桶
 *
 * 取得空桶，使用了一些奇怪的技巧。
 *
 * @param alloc_size 分配的块大小(指数形式)
 * @return empty_bucket
 */
struct bucket_desc* take_empty_bucket(uint8_t alloc_size) {
    struct bucket_desc *bucket;
    uint64_t bucket_page_addr = VIRTUAL(get_free_page());
    init_bucket_page(bucket_page_addr, alloc_size);
    if (alloc_size == SPECIAL_BUCKET_SIZE_LOG2) {
        bucket = (struct bucket_desc *) bucket_page_addr;
        bucket->refcnt = 1;
        bucket->freeidx = 1;
    } else {
        bucket = (struct bucket_desc *) kmalloc_i(sizeof(struct bucket_desc));
        bucket->refcnt = 0;
        bucket->freeidx = 0;
    }
    bucket->page = bucket_page_addr;
    bucket->next = bucket_dir[alloc_size - MIN_ALLOC_SIZE_LOG2];
    bucket_dir[alloc_size - MIN_ALLOC_SIZE_LOG2] = bucket;
    return bucket;
}

/**
 * @brief 申请一块内核内存
 *
 * 向内核申请size大小的一块内存，并返回该内存的起始地址；申请失败时返回NULL。
 * 所申请的内存至少为1B, 且不能超过一页大小。实际分配的内存大小为2的幂次。
 *
 * @param size 申请的内存大小(最大为PAGE_SIZE)
 * @return 所申请内存的起始地址
 */
void* kmalloc_i(uint64_t size) {
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
    if (!bucket) bucket = take_empty_bucket(alloc_size);
    /* 从桶中获取一个空闲块 */
    bucket->refcnt += 1;
    uint64_t free_block = bucket->page + (bucket->freeidx << alloc_size);
    bucket->freeidx = *((uint8_t *) free_block);
    return (void *) free_block;
}

/**
 * @brief 释放一块已申请内核内存
 *
 * 释放之前申请的size大小的一块内存，并返回该内存的起始地址；释放失败时返回0，
 * 释放成功时返回参数size的值。
 *
 * @param ptr 待释放的内存地址
 * @param size 申请的内存大小(为0时自动推算)
 * @return 释放的内存大小，若为0则表示释放失败
 */
uint64_t kfree_s_i(void* ptr, uint64_t size) {
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
                return 0;
        }
    } else {
        uint8_t aligned_size = q_pow2_factor(addr);
        if (aligned_size < MIN_ALLOC_SIZE_LOG2) return 0;
        if (aligned_size < MAX_ALLOC_SIZE_LOG2) alloc_size_end = aligned_size;
    }
    /* 找到该块所在的桶 */
    struct bucket_desc* bucket, *prev_bucket;
    uint64_t page_addr = (addr >> PAGE_SIZE_LOG2) << PAGE_SIZE_LOG2;
    uint8_t guess_alloc_size = alloc_size_start;
    while (guess_alloc_size <= alloc_size_end) {
        bucket = bucket_dir[guess_alloc_size - MIN_ALLOC_SIZE_LOG2];
        prev_bucket = NULL;
        while (bucket) {
            if (bucket->page == page_addr) break;
            prev_bucket = bucket;
            bucket = bucket->next;
        }
        if (bucket) break;
        guess_alloc_size += 1;
    }
    if(!bucket) return 0;
    /* 将该块放回桶中 */
    bucket->refcnt -= 1;
    if(bucket->refcnt > 1 || (bucket->refcnt == 1 && (uint64_t) bucket != page_addr)) {
        *((uint8_t *) ptr) = bucket->freeidx;
        bucket->freeidx = (addr - page_addr) >> guess_alloc_size;
    } else { /* 空桶 */
        if (prev_bucket) {
            prev_bucket->next = bucket->next;
        } else {
            bucket_dir[guess_alloc_size - MIN_ALLOC_SIZE_LOG2] = bucket->next;
        }
        free_page(PHYSICAL(page_addr));
        if ((uint64_t) bucket != page_addr) kfree_s_i(bucket, sizeof(struct bucket_desc));
    }
    return 1 << guess_alloc_size;
}

/**
 * @brief malloc.c测试用例
 *
 * test case for malloc.c
 *
 */
void malloc_test() {
    kputs("malloc_test(): running");
    int test_size[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
    for (int i = 0; i < 9; i += 1) {
        void* temp_ptr;
        for (int j = 0; j < PAGE_SIZE / test_size[i]; j++) {
            kfree_s(kmalloc(test_size[i]), test_size[i]);
        }
        temp_ptr = kmalloc(test_size[i] - 1);
        assert(kfree(temp_ptr) == test_size[i]);
        /* random malloc && free test */
        void* ptr_list[83 + 1031];
        for (int j = 0; j < 1031; j++) {
            ptr_list[j] = kmalloc(test_size[i]);
        }
        for (int j = 0; j < 83; j++) {
            kfree(ptr_list[(j * 0x10001 + 73) % 83]);
        }
        for (int j = 0; j < 83; j++) {
            ptr_list[j + 1031] = kmalloc(test_size[i]);
        }
        for (int j = 0; j < 1031; j++) {
            kfree(ptr_list[((j * 0x10001 + 73) % 1031) + 83]);
        }
    }
    kputs("malloc_test(): Passed");
}
#endif

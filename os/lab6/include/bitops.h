#ifndef __BITOPS__
#define __BITOPS__
#include <stddef.h>

// Check whether `val` is power of 2
#define pow_of_2(val)                                                          \
    ({                                                                         \
        uint64_t __val = (val);                                                \
        __val && !(__val & (__val - 1));                                       \
    })

#define __idx(i) ((i) / 8)
#define __off(i) ((i)&7)

#define set_bit(addr, off)                                                     \
    do {                                                                       \
        uint8_t *__addr = (uint8_t *)(addr);                                   \
        __addr[__idx((off))] |= (1U << (__off(off)));                          \
    } while (0)

#define clear_bit(addr, off)                                                   \
    do {                                                                       \
        uint8_t *__addr = (uint8_t *)(addr);                                   \
        __addr[__idx((off))] &= ~(1U << (__off(off)));                         \
    } while (0)

#define test_bit(addr, off)                                                    \
    ({                                                                         \
        uint8_t *__addr = (uint8_t *)(addr);                                   \
        !!(__addr[__idx((off))] & (1U << (__off(off))));                       \
    })

#define test_and_clear_bit(addr, off)                                          \
    ({                                                                         \
        int ret = test_bit((addr), (off));                                     \
        clear_bit((addr), (off));                                              \
        ret;                                                                   \
    })

#define test_and_set_bit(addr, off)                                            \
    ({                                                                         \
        int ret = test_bit((addr), (off));                                     \
        set_bit((addr), (off));                                                \
        ret;                                                                   \
    })

static inline int64_t find_next_zero_bit(void *addr, uint32_t size,
                                         uint32_t offset) {
    uint32_t i = offset;
    while (i < size) {
        if ((__off(i) == 0) && (*((uint8_t *)addr + __idx(i)) == 0xffU)) {
            i += 8;
            continue;
        }
        uint8_t val = *((uint8_t *)addr + __idx(i));
        for (uint32_t j = __off(i); (j < 8) && (i < size); ++j, ++i) {
            if (!test_bit(&val, j)) {
                return i;
            }
        }
    }
    return -1;
}

static inline uint64_t min_pow_of_2(uint64_t n) {
    // Most significant bit is 1
    if ((int64_t)n < 0) {
        return 1ULL << 63;
    }
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return (n + 1) >> 1;
}

#define FLOOR(addr, align) ((addr) & ~((align)-1))
#define CEIL(addr, align) (((addr) + (align)-1) & ~((align)-1))
#define ALIGN(addr, align) FLOOR((addr), (align))

#endif /* __BITOPS__ */

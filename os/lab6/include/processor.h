#ifndef __PROCESSOR_H__
#define __PROCESSOR_H__

#include <stddef.h>
#include <assert.h>

#define BYTES_PER_WORD 64U
static_assert((BYTES_PER_WORD >> 3) == sizeof(uint64_t), "not 64 bytes");

static inline uint32_t cpu_id() {
    return 0;
}

static inline uint32_t cache_line_size() {
    return 64;
}

#endif /* __PROCESSOR_H__ */

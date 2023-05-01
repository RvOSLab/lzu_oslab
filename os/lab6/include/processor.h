#ifndef __PROCESSOR_H__
#define __PROCESSOR_H__

#include <stddef.h>

static inline uint32_t cpu_id() {
    return 0;
}

static inline uint32_t cache_line_size() {
    return 64;
}

#endif /* __PROCESSOR_H__ */

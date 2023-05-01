#include <stddef.h>
#include <mm.h>
#include <compiler.h>
#include <slab.h>
#include <bitops.h>

#define BUFCTL_END (((kmem_bufctl_t)(~0U)) - 0)
#define BUFCTL_FREE (((kmem_bufctl_t)(~0U)) - 1)
#define SLAB_OBJ_LIMIT (((kmem_bufctl_t)(~0U)) - 2)
uint32_t off_slab_obj_limit;

struct kmalloc_info {
    char *names[NR_KMALLOC_TYPES];
    uint32_t size;
};

#define INIT_KMALLOC_INFO(__size, __readable_size)                             \
    {                                                                          \
        .names[KMALLOC_NORMAL] = "kmalloc-" #__readable_size,                  \
        .names[KMALLOC_DMA] = "kmalloc-dma-" #__readable_size, .size = __size, \
    }

struct kmalloc_info kmalloc_infos[NR_KMALLOC_CACHES] __initdata = {
    INIT_KMALLOC_INFO(16, 16),        INIT_KMALLOC_INFO(32, 32),
    INIT_KMALLOC_INFO(64, 64),        INIT_KMALLOC_INFO(128, 128),
    INIT_KMALLOC_INFO(256, 256),      INIT_KMALLOC_INFO(512, 512),
    INIT_KMALLOC_INFO(1024, 1K),      INIT_KMALLOC_INFO(2048, 2K),
    INIT_KMALLOC_INFO(4096, 4K),      INIT_KMALLOC_INFO(2 * 4096, 8K),
    INIT_KMALLOC_INFO(4 * 8092, 16K), INIT_KMALLOC_INFO(8 * 4096, 32K),
};

struct kmem_cache *kmalloc_caches[NR_KMALLOC_TYPES][NR_KMALLOC_CACHES];

struct kmem_cache kmem_cache = {
    .slab_partial = INIT_LINKED_LIST(kmem_cache.slab_partial),
    .slab_full = INIT_LINKED_LIST(kmem_cache.slab_full),
    .slab_empty = INIT_LINKED_LIST(kmem_cache.slab_full),
    .obj_size = sizeof(struct kmem_cache),
    .name = "kmem_cache",
};

struct objp_cache_init init_kmem_objp_cache __initdata = { {
    .capacity = BOOT_OBJP_CACHE_ENTRIES,
    .avail = 0,
    .batch_count = 1,
} };

struct objp_cache_init init_kmalloc_objp_cache __initdata = { {
    .capacity = BOOT_OBJP_CACHE_ENTRIES,
    .avail = 0,
    .batch_count = 1,
} };

struct linked_list_node cache_list = INIT_LINKED_LIST(cache_list);

#define __KMEM_OFF_SLAB 0x80000000U
static inline int off_slab(struct kmem_cache *kmem_cache) {
    return kmem_cache->flags & __KMEM_OFF_SLAB;
}

static void cache_estimate(uint32_t order, uint32_t obj_size, uint32_t align,
                           uint32_t flags, uint32_t *left_over,
                           uint32_t *slab_size, uint32_t *obj_num) {
    uint32_t block_size = 1 << order;
    obj_size = ALIGN(obj_size, align);
    uint32_t slabmgt_size = 0;
    uint32_t bufctl_size = sizeof(kmem_bufctl_t);
    uint32_t i = 0;
    uint32_t used_size = 0;
    if (flags & __KMEM_OFF_SLAB) {
        bufctl_size = 0;
    }
    while (used_size < block_size) {
        ++i;
        slabmgt_size = ALIGN(sizeof(struct slab) + bufctl_size * i, align);
        used_size = slabmgt_size + i * obj_size;
    }
    *obj_num = --i;
    *slab_size = ALIGN(sizeof(struct slab) + bufctl_size * i, align);
    *left_over = block_size - (*slab_size + i * obj_size);
}

struct kmem_cache *find_kmalloc_cache(uint64_t size, uint32_t flags) {
    uint32_t kmalloc_type = KMALLOC_NORMAL;
    if (flags & SLAB_CACHE_DMA) {
        kmalloc_type = KMALLOC_DMA;
    }
    for (int i = 0; i < NR_KMALLOC_CACHES; ++i) {
        if (kmalloc_caches[kmalloc_type][i]->obj_size >= size) {
            return kmalloc_caches[kmalloc_type][i];
        }
    }
    return NULL;
}

static void free_objp_block(struct kmem_cache *cachep, void *objpp[],
                            int32_t count) {
}

static struct objp_cache *alloc_objp_cache(int32_t capacity,
                                           int32_t batch_count) {
    struct objp_cache *p = (struct objp_cache *)kmalloc(
        sizeof(sizeof(struct objp_cache) + capacity * sizeof(p->data)),
        GFP_KERNEL);
    p->avail = 0;
    p->batch_count = batch_count;
    p->capacity = capacity;
    p->touched = 0;
    return p;
}
static void do_tune_obj_cache(struct kmem_cache *cachep, int32_t capacity,
                              int32_t batch_count) {
    struct objp_cache *new = alloc_objp_cache(capacity, batch_count);
    if (!new) {
        panic("do_tune_obj_cache: alloc_objp_cache() failed");
    }
    struct objp_cache *old = cachep->objp_cache;
    if (old) {
        free_objp_block(cachep, old->data, old->batch_count);
    }
    cachep->objp_cache = new;
    cachep->free_limit = 2 * cachep->objp_cache->batch_count + cachep->obj_num;
}

static void tune_objp_cache(struct kmem_cache *cachep) {
    int32_t capacity = 0;
    if (cachep->obj_size > (16 * PAGE_SIZE)) {
        capacity = 1;
    } else if (cachep->obj_size > PAGE_SIZE) {
        capacity = 8;
    } else if (cachep->obj_size > 1024) {
        capacity = 24;
    } else if (cachep->obj_size > 256) {
        capacity = 54;
    } else {
        capacity = 120;
    }

    do_tune_obj_cache(cachep, capacity, (capacity + 1) / 2);
}

static enum kmem_cache_state {
    NONE,
    PARTIAL,
    READY,
} kmem_cache_state = NONE;

// Initialize slab allocator.
//
// Initialization is difficult, requiring careful arrangement of cache
// initialization sequence.
//
// Steps:
// 1. Initialize `kmem_cache`. After that, we can allocate `kmem_cache_t`
//    descriptor. This stage is denoted as `NONE`.
// 2. Intialize the first kmalloc cache which is used to allocate `objp_cache`.
//    After that, we can allocate `struct objp_cache`. This stage is denoted
//    as `PARTIAL`.
// 3. Allocate all other kmalloc caches
// 4. Substitue __initdata data with kmalloc allocated data.
// 5. Resize cache's `objp_cache`. Ather that, `kmem_cache_init()` is finished
//    and `kmem_cache_state` enters `READY`.
void kmem_cache_init() __init {
    // 1. Initialize `kmem_cache`
    kmem_cache.objp_cache = (struct objp_cache *)&init_kmem_objp_cache;
    kmem_cache.color_off = cache_line_size();
    kmem_cache.obj_size = ALIGN(kmem_cache.obj_size, kmem_cache.color_off);
    if (kmem_cache.obj_size <= kmalloc_infos[0].size) {
        panic("sizeof(struct opj_objp_cache_init) <= kmalloc_infos[0].size");
    }
    uint32_t left_over = 0;
    cache_estimate(kmem_cache.order, kmem_cache.obj_size, cache_line_size(),
                   kmem_cache.flags, &left_over, &kmem_cache.slab_size,
                   &kmem_cache.obj_num);
    assert(kmem_cache.obj_num, "kmem_cache_init: kmem_cache obj_num is 0");
    kmem_cache.color = left_over / kmem_cache.color_off;
    kmem_cache.color_next = 0;
    linked_list_push(&cache_list, &kmem_cache.list);

    // 2&3. Initalize kmalloc caches
    for (uint32_t kmalloc_type = KMALLOC_NORMAL;
         kmalloc_type < NR_KMALLOC_TYPES; ++kmalloc_type) {
        for (uint32_t i = 0; i < NR_KMALLOC_CACHES; ++i) {
            struct kmem_cache *kmalloc_cache = kmem_cache_create(
                kmalloc_infos[kmalloc_type].names[i],
                kmalloc_infos[kmalloc_type].size, 0,
                kmalloc_type == KMALLOC_DMA ? SLAB_CACHE_DMA : 0, NULL, NULL);
            if (!off_slab(kmalloc_cache)) {
                off_slab_obj_limit =
                    (kmalloc_cache->obj_size - sizeof(struct slab)) /
                    sizeof(kmem_bufctl_t);
            }
            kmalloc_caches[kmalloc_type][i] = kmalloc_cache;
        }
    }

    // 4. Substitue __initdata data with kmalloc allocated data.
    void *p = kmalloc(sizeof(init_kmem_objp_cache), GFP_KERNEL);
    if (!p) {
        panic("kmem_cache_init: kmalloc failed");
    }
    memcpy(p, &init_kmem_objp_cache, sizeof(init_kmem_objp_cache));
    p = kmalloc(sizeof(init_kmalloc_objp_cache), GFP_KERNEL);
    if (!p) {
        panic("kmem_cache_init: kmalloc failed");
    }
    memcpy(p, &init_kmem_objp_cache, sizeof(init_kmem_objp_cache));

    // 5. Resize cache's `objp_cache`
    struct linked_list_node *list_node = NULL;
    for_each_linked_list_node(list_node, &cache_list) {
        struct kmem_cache *cachep =
            container_of(list_node, struct kmem_cache, list);
        tune_objp_cache(cachep);
    }

    // Finished!!!
    kmem_cache_state = READY;
}

struct kmem_cache *
kmem_cache_create(const char *name, uint64_t size, uint64_t align,
                  uint32_t flags,
                  void (*ctor)(void *, struct kmem_cache *, uint32_t),
                  void (*dtor)(void *, struct kmem_cache *, uint32_t)) {
}

void *kmem_cache_alloc(struct kmem_cache *cachep, uint32_t flags) {
}

void kmem_cache_free(struct kmem_cache *cachep, void *buf) {
}
void kmem_cache_destroy(struct kmem_cache *cachep) {
}

// TODO: handle kmalloc flags
void *kmalloc(uint64_t size, uint32_t flags) {
    return kmem_cache_alloc(find_kmalloc_cache(size, flags), flags);
}

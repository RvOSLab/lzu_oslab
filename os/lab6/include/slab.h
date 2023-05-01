#ifndef __SLAB_H__
#define __SLAB_H__

#include <stddef.h>
#include <mmzone.h>
#include <page_alloc.h>
#include <compiler.h>
#include <assert.h>

struct objp_cache {
    int32_t capacity;
    int32_t avail;
    int32_t batch_count;
    int32_t touched;
    void *data[0];
};

#define BOOT_OBJP_CACHE_ENTRIES 1
struct objp_cache_init {
    struct objp_cache objp_cache;
    void *data[BOOT_OBJP_CACHE_ENTRIES];
};

typedef uint16_t kmem_bufctl_t;

struct kmem_cache {
    uint32_t flags;

    uint32_t obj_size;
    uint32_t obj_num;
    uint32_t slab_size;
    uint32_t free_limit;

    struct kmem_cache *slabp_cache;

    struct objp_cache *objp_cache;

    gfp_t gfpflags;
    uint32_t order;

    struct linked_list_node slab_full;
    struct linked_list_node slab_empty;
    struct linked_list_node slab_partial;
    uint32_t free_objs;

    uint32_t color;
    uint32_t color_off;
    uint32_t color_next;

    void (*ctor)(void *, struct kmem_cache *, uint32_t);
    void (*dtor)(void *, struct kmem_cache *, uint32_t);

    const char *name;
    struct linked_list_node list;
};

struct kmem_cache *
kmem_cache_create(const char *name, uint64_t size, uint64_t align,
                  uint32_t flags,
                  void (*ctor)(void *, struct kmem_cache *, uint32_t),
                  void (*dtor)(void *, struct kmem_cache *, uint32_t));
void *kmem_cache_alloc(struct kmem_cache *cachep, uint32_t flags);
void kmem_cache_free(struct kmem_cache *cachep, void *buf);
void kmem_cache_destroy(struct kmem_cache *cachep);

struct slab {
    uint32_t color_off;
    void *mem;
    uint32_t inuse;
    struct linked_list_node list;
    kmem_bufctl_t freelist;
};

enum kmalloc_type {
    KMALLOC_NORMAL,
    KMALLOC_DMA,
    NR_KMALLOC_TYPES,
};
#define NR_KMALLOC_CACHES 12
extern struct kmem_cache *kmalloc_caches[NR_KMALLOC_TYPES][NR_KMALLOC_CACHES];

// kmem_cache_create() flags
#define SLAB_POSION 0x01U // Posion slab objs
#define SLAB_RED_ZONE 0x02U // Eanble red zone
#define SLAB_TRACE_USER 0x04U // Store last user in debug area
#define SLAB_CACHE_DMA 0x08U // Allocate memory in DMA zone
#define SLAB_PANIC 0x10U // Panic if `kmem_cache_create()` fails
#define SLAB_DEBUG (SLAB_PANIC | SLAB_POSION | SLAB_RED_ZONE | SLAB_TRACE_USER)
#define SLAB_HW_CACHE_ALIGN 0x20U // Align to hardware cache line
#define SLAB_ALLOWD_FLAGS                                                      \
    (SLAB_POSION | SLAB_RED_ZONE | SLAB_TRACE_USER | SLAB_CACHE_DMA |          \
     SLAB_PANIC | SLAB_HW_CACHE_ALIGN)

// `kmalloc()` allocates byte-sized chunk
extern void *kmalloc(uint64_t size, uint32_t flags);

#endif /* __SLAB_H__ */

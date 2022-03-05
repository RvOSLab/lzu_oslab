#ifndef DEVICE_FDT_H
#define DEVICE_FDT_H

#include <stddef.h>
#include <string.h>

typedef uint32_t fdt32_t;

#define FDT32(val) ((val & 0xff) << 24 | ((val >> 8) & 0xff) << 16 | ((val >> 16) & 0xff) << 8 | ((val >> 24) & 0xff))

enum fdt_config {
    FDT_MAGIC = FDT32(0xD00DFEED),
    FDT_DATA_ALIGN = sizeof(fdt32_t) - 1,
    FDT_V1_SIZE = 7 * sizeof(fdt32_t),
    FDT_V2_SIZE = FDT_V1_SIZE + sizeof(fdt32_t),
    FDT_V3_SIZE = FDT_V2_SIZE + sizeof(fdt32_t),
    FDT_V17_SIZE = FDT_V3_SIZE + sizeof(fdt32_t)
};

enum fdt_tag {
    FDT_BEGIN_NODE = FDT32(0x1),
    FDT_END_NODE = FDT32(0x2),
    FDT_PROP = FDT32(0x3),
    FDT_NOP = FDT32(0x4),
    FDT_END = FDT32(0x9)
};

struct fdt_header {
    fdt32_t magic; /* magic word FDT_MAGIC */
    fdt32_t totalsize; /* total size of DT block */
    fdt32_t off_dt_struct; /* offset to structure */
    fdt32_t off_dt_strings; /* offset to strings */
    fdt32_t off_mem_rsvmap; /* offset to memory reserve map */
    fdt32_t version; /* format version */
    fdt32_t last_comp_version; /* last compatible version */
    /* version 2 fields below */
    fdt32_t boot_cpuid_phys; /* Which physical CPU id we're booting on */
    /* version 3 fields below */
    fdt32_t size_dt_strings; /* size of the strings block */
    /* version 17 fields below */
    fdt32_t size_dt_struct; /* size of the structure block */
};

struct fdt_node_header {
    fdt32_t tag;
    char name[0];
};

struct fdt_property {
    fdt32_t tag;
    fdt32_t length;
    fdt32_t name_offset;
    char data[0];
};

union fdt_walk_pointer {
    struct fdt_node_header *node;
    struct fdt_property *prop;
    uint64_t address;
};

static inline uint32_t fdt32_to_cpu(fdt32_t val) {
    return (val & 0xff) << 24 | ((val >> 8) & 0xff) << 16 | ((val >> 16) & 0xff) << 8 | ((val >> 24) & 0xff);
}

static inline uint32_t fdt_align_length(uint32_t length) {
    return (length + FDT_DATA_ALIGN) & ~FDT_DATA_ALIGN;
}

static inline uint32_t fdt_get_prop_value_len(const struct fdt_property *prop) {
    return fdt32_to_cpu(prop->length);
}

static inline void fdt_walk_prop(union fdt_walk_pointer *pointer) {
    pointer->address += fdt_align_length(fdt_get_prop_value_len(pointer->prop));
    pointer->address += sizeof(struct fdt_property);
}

static inline void fdt_skip_props(union fdt_walk_pointer *pointer) {
    while (pointer->prop->tag == FDT_PROP) {
        fdt_walk_prop(pointer);
    }
}

// 返回值说明
// pointer->address != 0 && parent != 0 到达当前节点的子节点
// pointer->address != 0 && parent == 0 到达当前节点的兄弟节点 || 该节点再无兄弟节点
//     需根据pointer->node->tag判断(FDT_BEGIN_NODE || FDT_END_NODE)
// pointer->address == 0 && parent != 0 在当前节点未闭合时意外遇到FDT_END
// pointer->address == 0 && parent == 0 FDT已遍历完毕
static inline struct fdt_node_header *fdt_walk_node(union fdt_walk_pointer *pointer) {
    struct fdt_node_header *parent = NULL;
    if (pointer->node->tag == FDT_BEGIN_NODE) {
        parent = pointer->node;
        uint32_t name_length = strlen(pointer->node->name) + 1;
        name_length = fdt_align_length(name_length);
        pointer->address += name_length ? name_length : sizeof(fdt32_t);
    } // or FDT_END_NODE
    pointer->address += sizeof(struct fdt_node_header);
    while (1) {
        if (pointer->node->tag == FDT_BEGIN_NODE) {
            break;
        } else if (pointer->node->tag == FDT_PROP) {
            fdt_skip_props(pointer);
        } else if (pointer->node->tag == FDT_END) {
            pointer->address = 0;
            return parent;
        } else if (pointer->node->tag == FDT_END_NODE) {
            if (!parent) break;
            parent = NULL;
            pointer->address += sizeof(struct fdt_node_header);
        } else { // or FDT_NOP
            pointer->address += sizeof(struct fdt_node_header);
        }
    }
    return parent;
}

// 返回值说明
// deepth == 0 && pointer->address == 0 传入的节点完整，且FDT遍历结束
// deepth != 0 && pointer->address == 0 传入的节点不完整，因FDT遍历完毕终止
// deepth == 0 && pointer->address != 0 已转到当前节点的兄弟节点 || 该节点再无兄弟节点
//     需根据pointer->node->tag判断(FDT_BEGIN_NODE || FDT_END_NODE)
// deepth != 0 && pointer->address != 0 未知情况
static inline uint32_t fdt_skip_node(union fdt_walk_pointer *pointer) {
    struct fdt_node_header *parent = NULL;
    uint32_t deepth = 0;
    parent = fdt_walk_node(pointer);
    while (pointer->address) {
        if (parent) {
            if (pointer->node->tag == FDT_END_NODE) break;
            deepth += 1;
        } else {
            if (deepth == 0) break;
            if (pointer->node->tag == FDT_END_NODE) deepth -= 1;
        }
        parent = fdt_walk_node(pointer);
    }
    return deepth;
}

static inline struct fdt_node_header *fdt_find_node_by_path(const struct fdt_header *fdt, const char* path) {
    char sep = '/';
    if (!path || !fdt || *path != sep) return NULL;
    union fdt_walk_pointer pointer = { .address = (uint64_t)fdt };
    pointer.address += fdt32_to_cpu(fdt->off_dt_struct);
    const char *search = path + 1;
    while (*search) { // 仍有路径待匹配
        /* 下阶段需匹配的范围 */
        const char *end = strchr(search, sep);
        if (!end) end = search + strlen(search);
        /* 下阶段需要的第一个节点 */
        fdt_walk_node(&pointer);
        while (1) { // 寻找名称匹配的兄弟节点
            const char *search_start = search;
            const char *node_name = pointer.node->name;
            while (search != end && *search == *node_name) { // 匹配字符串
                search += 1;
                node_name += 1;
            }
            if (search == end) break; // 匹配成功
            /* 匹配失败，切换到兄弟节点 */
            search = search_start;
            uint32_t deepth = fdt_skip_node(&pointer);
            if (deepth) return NULL;
            if (!pointer.address) return NULL;
            if (pointer.node->tag != FDT_BEGIN_NODE) return NULL;
        }
        if (*search == sep) { // such as /path/matched/...
            search += 1;
            if (!(*search)) return NULL; // 以sep结尾
        }
    }
    return pointer.node;
}

static inline const char *fdt_get_prop_name(const struct fdt_header *fdt, fdt32_t name_offset) {
    uint64_t string_address = (uint64_t) fdt;
    string_address += fdt32_to_cpu(fdt->off_dt_strings);
    string_address += fdt32_to_cpu(name_offset);
    return (const char *) string_address;
}

static inline struct fdt_property *fdt_get_props_of_node(struct fdt_node_header *node) {
    union fdt_walk_pointer pointer = { .node = node };
    uint32_t name_length = strlen(pointer.node->name) + 1;
    name_length = fdt_align_length(name_length);
    pointer.address += name_length ? name_length : sizeof(fdt32_t);
    pointer.address += sizeof(struct fdt_node_header);
    return pointer.prop;
}

static inline struct fdt_property *fdt_get_prop(const struct fdt_header *fdt, struct fdt_node_header *node, const char* name) {
    union fdt_walk_pointer pointer = { .node = node };
    pointer.prop = fdt_get_props_of_node(pointer.node);
    while (pointer.prop->tag == FDT_PROP) {
        const char *prop_name = fdt_get_prop_name(fdt, pointer.prop->name_offset);
        if (!strcmp(prop_name, name)) return pointer.prop;
        fdt_walk_prop(&pointer);
    }
    return NULL;
}

static inline uint32_t fdt_get_prop_num_value(const struct fdt_property *prop, uint32_t idx) {
    fdt32_t *values = (fdt32_t *)(prop->data);
    return fdt32_to_cpu(values[idx]);
}

static inline const char *fdt_get_prop_str_value(const struct fdt_property *prop, uint32_t offset) {
    const char *str = (const char *)prop->data;
    return str + offset;
}

static inline fdt32_t fdt_get_prop_phandle_value(const struct fdt_property *prop, uint32_t idx) {
    fdt32_t *values = (fdt32_t *)(prop->data);
    return values[idx];
}

static inline struct fdt_node_header *fdt_find_node_by_phandle(const struct fdt_header *fdt, fdt32_t phandle) {
    union fdt_walk_pointer pointer = { .address = (uint64_t)fdt };
    pointer.address += fdt32_to_cpu(fdt->off_dt_struct);
    while (pointer.address) {
        if (pointer.node->tag == FDT_BEGIN_NODE) {
            struct fdt_property *prop = fdt_get_prop(fdt, pointer.node, "phandle");
            if (prop) {
                if (fdt_get_prop_value_len(prop) != sizeof(fdt32_t)) return NULL;
                if (fdt_get_prop_phandle_value(prop, 0) == phandle) return pointer.node;
            }
        }
        fdt_walk_node(&pointer);
    }
    return NULL;
}

#endif

#include <stddef.h>
#ifndef __DTB_H__
#define __DTB_H__

enum fdt_config {
    FDT_MAGIC = 0xd00dfeed,
    FDT_FIRST_SUPPORTED_VERSION = 2,
    FDT_LAST_SUPPORTED_VERSION = 17,
    FDT_4B_ALIGN = 0x3,
    FDT_V1_SIZE = 7 * sizeof(uint32_t),
    FDT_V2_SIZE = FDT_V1_SIZE + sizeof(uint32_t),
    FDT_V3_SIZE = FDT_V2_SIZE + sizeof(uint32_t),
    FDT_V17_SIZE = FDT_V3_SIZE + sizeof(uint32_t),
};

enum dt_tag {
    DT_BEGIN_NODE = 0x1,
    DT_END_NODE = 0x2,
    DT_PROP = 0x3,
    DT_NOP = 0x4,
    DT_END = 0x9,
};

struct fdt_header {
    uint32_t magic; /* magic word FDT_MAGIC */
    uint32_t totalsize; /* total size of DT block */
    uint32_t off_dt_struct; /* offset to structure */
    uint32_t off_dt_strings; /* offset to strings */
    uint32_t off_mem_rsvmap; /* offset to memory reserve map */
    uint32_t version; /* format version */
    uint32_t last_comp_version; /* last compatible version */
    /* version 2 fields below */
    uint32_t boot_cpuid_phys; /* Which physical CPU id we're booting on */
    /* version 3 fields below */
    uint32_t size_dt_strings; /* size of the strings block */
    /* version 17 fields below */
    uint32_t size_dt_struct; /* size of the structure block */
};

struct property {
    const char *name; /* name of property */
    uint32_t length; /* length of property value */
    const void *value; /* property value */
    struct property *next; /* next property */
};
struct device_node {
    const char *name; /* node的名称，取最后一次“/”和“@”之间子串 */
    const char *type; /* device_type的属性名称，没有为<NULL> */
    // uint32_t phandle;      /* phandle属性值 */
    const char *
            full_name; /* 指向该结构体结束的位置，存放node的路径全名，例如：/chosen */
    // struct fwnode_handle fwnode;

    struct property *
            properties; /* 指向该节点下的第一个属性，其他属性与该属性链表相接 */
    // struct property *deadprops;  /* removed properties */
    struct device_node *parent; /* 父节点 */
    struct device_node *child; /* 子节点 */
    struct device_node *sibling; /* 姊妹节点，与自己同等级的node */
    uint32_t depth; /* 节点的深度 */
    void *data;
};

void unflatten_device_tree(const void *fdt_start);

#endif

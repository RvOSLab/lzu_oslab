#include <stddef.h>
#include <kdebug.h>
#include <dtb.h>
#include <string.h>

struct device_node node[100];
struct property prop[150];
int64_t node_used = -1;
int64_t prop_used = -1;

size_t fdt_header_size_(uint32_t version)
{
    if (version <= 1)
        return FDT_V1_SIZE;
    if (version <= 2)
        return FDT_V2_SIZE;
    if (version <= 3)
        return FDT_V3_SIZE;
    return FDT_V17_SIZE;
}

uint32_t check_fdt_header(const void *fdt_start)
{
    const struct fdt_header *header = (const struct fdt_header *)fdt_start;
    const struct fdt_header header_bswap = {
        .magic = fdt32_to_cpu(header->magic),
        .totalsize = fdt32_to_cpu(header->totalsize),
        .off_dt_struct = fdt32_to_cpu(header->off_dt_struct),
        .off_dt_strings = fdt32_to_cpu(header->off_dt_strings),
        .off_mem_rsvmap = fdt32_to_cpu(header->off_mem_rsvmap),
        .version = fdt32_to_cpu(header->version),
        .last_comp_version = fdt32_to_cpu(header->last_comp_version),
        .boot_cpuid_phys = fdt32_to_cpu(header->boot_cpuid_phys),
        .size_dt_strings = fdt32_to_cpu(header->size_dt_strings),
        .size_dt_struct = fdt32_to_cpu(header->size_dt_struct),
    };
    if (header_bswap.magic != FDT_MAGIC) {
        kprintf("Invalid magic number 0x%x\n", header_bswap.magic);
        return -1;
    }
    if ((header_bswap.version < FDT_FIRST_SUPPORTED_VERSION) ||
        (header_bswap.last_comp_version > FDT_LAST_SUPPORTED_VERSION)) {
        kprintf("Unsupported version (0x%x) or compat version (0x%x)\n",
                header_bswap.version, header_bswap.last_comp_version);
        return -1;
    }
    if (header_bswap.version < header_bswap.last_comp_version) {
        kprintf("Error: version 0x%x < compat version 0x%x\n",
                header_bswap.version, header_bswap.last_comp_version);
        return -1;
    }
    if ((header_bswap.off_dt_struct >= header_bswap.totalsize) ||
        (header_bswap.off_dt_struct <=
         fdt_header_size_(header_bswap.version))) {
        kprintf("Invalid struct offset 0x%x\n", header_bswap.off_dt_struct);
        return -1;
    }
    if ((header_bswap.off_dt_strings >= header_bswap.totalsize) ||
        (header_bswap.off_dt_strings <=
         fdt_header_size_(header_bswap.version))) {
        kprintf("Invalid string offset 0x%x\n", header_bswap.off_dt_strings);
        return -1;
    }

    kprintf("Magic number 0x%x\n", header_bswap.magic);
    kprintf("Total size of DT block: 0x%x\n", header_bswap.totalsize);
    kprintf("Offset to structure: 0x%x\n", header_bswap.off_dt_struct);
    kprintf("Offset to strings: 0x%x\n", header_bswap.off_dt_strings);
    kprintf("Offset to memory reserve map: 0x%x\n",
            header_bswap.off_mem_rsvmap);
    kprintf("Format version: 0x%x\n", header_bswap.version);
    kprintf("Last compatible version: 0x%x\n", header_bswap.last_comp_version);
    kprintf("Boot CPU physical ID: 0x%x\n", header_bswap.boot_cpuid_phys);
    kprintf("Size of the strings block: 0x%x\n", header_bswap.size_dt_strings);
    kprintf("Size of the structure block: 0x%x\n", header_bswap.size_dt_struct);

    return 0;
}

uint64_t unflatten_dt_nodes(const void *fdt_start)
{
    const struct fdt_header *header = fdt_start;
    const void *dt_struct_start =
            fdt_start + fdt32_to_cpu(header->off_dt_struct);
    const char *dt_string_start =
            fdt_start + fdt32_to_cpu(header->off_dt_strings);

    const void *dt_struct_pointer = dt_struct_start;
    uint64_t depth = 0;

    while (fdt32_to_cpu(*(uint32_t *)dt_struct_pointer) == DT_BEGIN_NODE) {
        depth++;
        uint32_t prop_in_node = 0;
        node_used++;
        node[node_used].name = NULL;
        node[node_used].type = NULL;
        node[node_used].full_name = NULL;
        node[node_used].properties = NULL;
        node[node_used].parent = NULL;
        node[node_used].sibling = NULL;
        node[node_used].child = NULL;
        node[node_used].data = NULL;
        node[node_used].depth = depth;

        if (node_used) {
            if (node[node_used].depth > node[node_used - 1].depth) {
                node[node_used - 1].child = &node[node_used];
                node[node_used].parent = &node[node_used - 1];
            } else if (node[node_used].depth < node[node_used - 1].depth) {
                node[node_used - 1].parent->sibling = &node[node_used];
            } else {
                node[node_used - 1].sibling = &node[node_used];
            }
        }

        // read node name
        dt_struct_pointer += 4;
        node[node_used].name = (const char *)dt_struct_pointer;
        if (node[node_used].name[0] == '\0') {
            if (node_used == 0)
                node[node_used].name = "/";
            //kprintf("node name is empty\n");
            dt_struct_pointer++;
        } else
            //kprintf("node name is %s\n", node[node_used].name);
            for (; *(const char *)dt_struct_pointer; dt_struct_pointer++)
                ;
        dt_struct_pointer++;
        for (; (uint64_t)dt_struct_pointer & FDT_4B_ALIGN; dt_struct_pointer++)
            ;

        // read properties
        while (fdt32_to_cpu(*(uint32_t *)dt_struct_pointer) == DT_PROP) {
            //kprintf("in a property\n");
            prop_used++;
            if (!prop_in_node)
                node[node_used].properties = &prop[prop_used];
            // read property length
            dt_struct_pointer += 4;
            prop[prop_used].length =
                    fdt32_to_cpu(*(uint32_t *)dt_struct_pointer);
            //kprintf("property length: %d\n", prop[prop_used].length);

            // read property name
            dt_struct_pointer += 4;
            prop[prop_used].name =
                    (const char *)(dt_string_start +
                                   fdt32_to_cpu(
                                           *(uint32_t *)dt_struct_pointer));

            //kprintf("property name: %s\n", prop[prop_used].name);

            // read property value
            dt_struct_pointer += 4;
            prop[prop_used].value = dt_struct_pointer;
            if (strcmp(prop[prop_used].name, "device_type") == 0)
                node[node_used].type = (const char *)(prop[prop_used].value);
            //kprintf("property value as string: %s\n", (const char *)prop[prop_used].value);
            //kprintf("property value as uint32: 0x%x\n", fdt32_to_cpu(*(uint32_t *)prop[prop_used].value));
            dt_struct_pointer += prop[prop_used].length;
            for (; (uint64_t)dt_struct_pointer & FDT_4B_ALIGN;
                 dt_struct_pointer++)
                ;

            // read next property
            if (fdt32_to_cpu(*(uint32_t *)dt_struct_pointer) == DT_PROP)
                prop[prop_used].next = &prop[prop_used + 1];
            else
                prop[prop_used].next = NULL;

            prop_in_node++;
        }
        while (fdt32_to_cpu(*(uint32_t *)dt_struct_pointer) == DT_END_NODE) {
            depth--;
            dt_struct_pointer += 4;
        }
    }
    /** 校验OF_DT_END。 */
    if (fdt32_to_cpu(*(uint32_t *)dt_struct_pointer) != DT_END) {
        kprintf("invalid end of dt\n");
        return -1;
    }
    return 0;
}
void print_node_info(const struct device_node *node_i)
{
    char tab[10] = "         ";
    kprintf("%s%s\n", tab + 9 - node_i->depth, node_i->name);
    if (node_i->type)
        kprintf("%s- type: %s\n", tab + 9 - node_i->depth, node_i->type);
    if (node_i->full_name)
        kprintf("%s- full_name: %s\n", tab + 9 - node_i->depth,
                node_i->full_name);
    if (node_i->properties) {
        struct property *prop_i = node_i->properties;
        while (prop_i) {
            kprintf("%s- %s: ", tab + 8 - node_i->depth, prop_i->name);
            if (prop_i->value)
                kprintf("%s\n", (const char *)prop_i->value);
            else
                kprintf("NULL\n");
            prop_i = prop_i->next;
        }
    }
    if (node_i->child) {
        struct device_node *child_i = node_i->child;
        while (child_i) {
            print_node_info(child_i);
            child_i = child_i->sibling;
        }
    }
}

void unflatten_device_tree(const void *fdt_start)
{
    /** 检查头部信息 */
    if (check_fdt_header(fdt_start) == -1) {
        kprintf("FDT Header check failed.\n");
        return;
    }
    /** 开冲 */
    unflatten_dt_nodes(fdt_start);
    /** 打印节点信息 */
    print_node_info(&node[0]);
}

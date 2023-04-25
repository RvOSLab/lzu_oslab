#ifndef __ASM_PAGE__
#define __ASM_PAGE__

#ifndef __ASSEMBLY__
#define __START_KERNEL 0xffffffff80200000UL
#define __START_KERNEL_MAP 0xffffffff00000000UL
#define __PROT_LEAF 0x0fUL
#else
#define __START_KERNEL 0xffffffff80200000
#define __START_KERNEL_MAP 0xffffffff00000000
#define __PROT_LEAF 0x0f
#endif /* __ASSEMBLY__ */

#define MAX_SBI_ADDR 0x80200000UL

#define DMA_ZONE_SIZE (32UL << 20)

#define PAGE_OFFSET 0xffffffd800000000UL
#define DIRECT_MAPPING_SIZE (1UL << 37)
#define DIRECT_MAPPING_END (PAGE_OFFSET + DIRECT_MAPPING_SIZE)

#define VMALLOC_SIZE (1UL << 36)
#define VMALLOC_START (PAGE_OFFSET - VMALLOC_SIZE)
#define VMALLOC_END PAGE_OFFSET

#define __floor(addr, align) ((addr) & ~((align)-1))

#endif /* __ASM_PAGE__ */

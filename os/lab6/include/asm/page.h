#ifndef __ASM_PAGE__
#define __ASM_PAGE__

#ifndef __ASSEMBLY__
#define __START_KERNEL 0xffffffff80200000UL
#define __START_KERNEL_MAP 0xffffffff00000000UL

#define __PAGE_SIZE (0x1 << 12)

#define __PROT_LEAF 0x0fUL
#else
#define __START_KERNEL 0xffffffff80200000
#define __START_KERNEL_MAP 0xffffffff00000000

#define __PAGE_SIZE (0x1UL << 12)

#define __PROT_LEAF 0x0f
#endif /* __ASSEMBLY__ */

#define FLOOR(addr, align) ((addr) / (align) * (align)) // 向下取整到 align

#endif /* __ASM_PAGE__ */

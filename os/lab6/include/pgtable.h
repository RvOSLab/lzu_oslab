#ifndef __PGTABLE__
#define __PGTABLE__

#include <asm/page.h>
#include <stddef.h>

// Master kenrl page table
extern uint64_t boot_pg_dir[512];

// Virtual address <-> Physical address
#define va(paddr) ((uint64_t)(paddr) + PAGE_OFFSET)
#define pa(vaddr)                                                              \
    ({                                                                         \
        uint64_t __vaddr = (uint64_t)(vaddr);                                  \
        (__vaddr >= __START_KERNEL ? __vaddr - __START_KERNEL_MAP :            \
                                     __vaddr - PAGE_OFFSET);                   \
    })

// SV39 three-level page table
#define PAGE_SHIFT 12
#define PAGE_SIZE (0x1UL << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1))

#define PTE_SHIFT PAGE_SHIFT
#define PTE_SIZE PAGE_SIZE
#define PTE_MASK PAGE_MASK
#define PTE_FLAG_BITS 10

#define PMD_SHIFT 21
#define PMD_SIZE (0x1UL << PMD_SHIFT)
#define PMD_MASK (~(PMD_SIZE - 1))
#define PMD_FLAG_BITS 10

#define PGD_SHIFT 30
#define PGD_SIZE (0x1UL << PGD_SHIFT)
#define PGD_MASK (~(PGD_SIZE - 1))
#define PGD_FLAG_BITS 10

#define PTRS_PER_PGD 512
#define PTRS_PER_PTE 512
#define PTRS_PER_PMD 512

#define PROT_DIRTY 0x80U
#define PROT_ACCESSED 0x40U
#define PROT_USER 0x10U
#define PROT_RD 0x02U
#define PROT_WR 0x04U
#define PROT_EXE 0x08U
#define PROT_VALID 0x01U
#define PROT_PRESENT PROT_VALID

#define PROT_RW (PROT_RD | PROT_WR)
#define PROT_LEAF (PROT_RD | PROT_WR | PROT_EXE | PROT_PRESENT)
#define PROT_KERNEL (PROT_RD | PROT_WR | PROT_PRESENT)

// Get PGD/PMT/PTE entry offset of virtual address `addr` in struct mm `mm`
//
// pgd_offset(addr) is the offset of the PGD table entry at `addr`
// pgd_entry(mm, addr) is the pointer of the PGD entry of `addr`
// pgd_val(mm, addr) is the address field of the PGD entry of `addr`
// pgd_page(mm, addr) is the pointer of the page which the PGD entry points to
// pgd_entry_kernel(addr) is the pointer of the master kernel PGD entry of
// `addr`
//
// Same for other function macros.
#define pgd_t uint64_t
#define pmd_t uint64_t
#define pte_t uint64_t

#define pgd_offset(addr) ((addr >> PGD_SHIFT) & (PTRS_PER_PGD - 1))
#define pmd_offset(addr) ((addr >> PMD_SHIFT) & (PTRS_PER_PMD - 1))
#define pte_offset(addr) ((addr >> PTE_SHIFT) & (PTRS_PER_PTE - 1))

// static inline pgd_t *pgd_entry(struct mm *mm, uint64_t addr) {
//   return mm->pgd + ((addr >> PGD_SHIFT) & (PTRS_PER_PGD - 1));
// }
//
// static inline uint64_t pgd_val(struct mm *mm, uint64_t addr) {
//   return ((*pgd_entry(mm, addr) << (PAGE_SHIFT - PGD_FLAG_BITS)) &
//   PAGE_MASK);
// }
//
// static inline uint64_t *pgd_page(struct mm *mm, uint64_t addr) {
//   return (uint64_t *)pgd_val(mm, addr);
// }
//
// static inline pmd_t *pmd_entry(struct mm *mm, uint64_t addr) {
//   return pgd_page(mm, addr) + pmd_offset(addr);
// }
//
// static inline uint64_t pmd_val(struct mm *mm, uint64_t addr) {
//   return (*pmd_entry(mm, addr) << (PAGE_SHIFT - PMD_FLAG_BITS)) & PAGE_MASK;
// }
//
// static inline uint64_t *pmd_page(struct mm *mm, uint64_t addr) {
//   return (uint64_t *)pmd_val(mm, addr);
// }
//
// static inline pte_t *pte_entry(struct mm *mm, uint64_t addr) {
//   return pgd_page(mm, addr) + pte_offset(addr);
// }
//
// static inline uint64_t pte_val(struct mm *mm, uint64_t addr) {
//   return (*pte_entry(mm, addr) << (PAGE_SHIFT - PTE_FLAG_BITS)) & PAGE_MASK;
// }
//
// static inline uint64_t *pte_page(struct mm *mm, uint64_t addr) {
//   return (uint64_t *)pte_val(mm, addr);
// }
//
// static inline pgd_t *pgd_entry_kernel(uint64_t addr) {
//   return pgd_entry(&init_mm, addr);
// }
//
// static pgd_t mkpgd(uint64_t addr, uint8_t prot) {
//   return ((addr >> PAGE_SHIFT) << PGD_FLAG_BITS) | prot;
// }
// static pmd_t mkpmd(uint64_t addr, uint8_t prot) {
//   return ((addr >> PAGE_SHIFT) << PMD_FLAG_BITS) | prot;
// }
// static pte_t mkpte(uint64_t addr, uint8_t prot) {
//   return ((((addr) >> PAGE_SHIFT) << PTE_FLAG_BITS) | prot);
// }

#define pgd_entry(mm, addr)                                                    \
    ((mm)->pgd + (((addr) >> PGD_SHIFT) & (PTRS_PER_PGD - 1)))

#define pgd_page(mm, addr)                                                     \
    ((*pgd_entry((mm), (addr)) << (PAGE_SHIFT - PGD_FLAG_BITS)) & PAGE_MASK)

#define pmd_entry(mm, addr)                                                    \
    ((uint64_t *)pgd_page((mm), (addr)) + pmd_offset((addr)))

#define pmd_page(mm, addr)                                                     \
    ((*pmd_entry((mm), (addr)) << (PAGE_SHIFT - PMD_FLAG_BITS)) & PAGE_MASK)

#define pte_entry(mm, addr)                                                    \
    ((uint64_t *)pmd_page((mm), (addr)) + pte_offset((addr)))

#define pte_page(mm, addr)                                                     \
    ((*pte_entry((mm), (addr)) << (PAGE_SHIFT - PTE_FLAG_BITS)) & PAGE_MASK)

#define pgd_entry_kernel(addr) pgd_entry(&init_mm, (addr))

#define mkpgd(paddr, prot) ((((paddr) >> PAGE_SHIFT) << PGD_FLAG_BITS) | (prot))
#define mkpmd(paddr, prot) ((((paddr) >> PAGE_SHIFT) << PMD_FLAG_BITS) | (prot))
#define mkpte(paddr, prot) ((((paddr) >> PAGE_SHIFT) << PTE_FLAG_BITS) | (prot))

#endif /* __PGTABLE__ */

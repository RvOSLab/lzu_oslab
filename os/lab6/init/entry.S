#include <asm/page.h>

    .globl boot_stack, boot_stack_top, _start, boot_pg_dir
    .section .text.entry

_start:
    la t0, boot_pg_dir
    srli t0, t0, 12
    li t1, (8 << 60)
    or t0, t0, t1
    csrw satp, t0
    sfence.vma

    # 修改 PC 为虚拟地址
    lla t0, next
    li t1, __START_KERNEL_MAP
    add t0, t0, t1
    jalr x0, t0, 0
next:
    la sp, boot_stack_top
    call main

    .section .data
boot_stack:
    .space 4096 * 4
boot_stack_top:

# 创建内核镜像区映射
boot_pg_dir:
    .zero 2 * 8
    .quad (__floor(__START_KERNEL - __START_KERNEL_MAP, 1 << 30) >> 2) | __PROT_LEAF
    .zero 507 * 8
    .quad (__floor(__START_KERNEL - __START_KERNEL_MAP, 1 << 30) >> 2) | __PROT_LEAF
    .quad (__floor(__START_KERNEL - __START_KERNEL_MAP, 1 << 30) >> 2) | __PROT_LEAF

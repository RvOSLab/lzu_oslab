    .globl boot_stack, boot_stack_top, _start
    .section .text.entry
_start:
    # 计算出 satp 寄存器的值
    # 使用 SV39 分页（39 位地址空间）
    la t0, boot_table
    srli t0, t0, 12
    li t1, 8
    slli t1, t1, 60
    or t0, t0, t1
    # 写入 satp 寄存器
    csrw satp, t0
    # 刷新整个 TLB
    sfence.vma

    la sp, boot_stack_top
    call main

    .section .bss
boot_stack:
    .space 4096 * 16
boot_stack_top:

    .section .data
# 开启大页模式，将线性地址 0x8000_0000 之后的 1GB 映射到物理地址 0x8000_0000
boot_table:
    .quad 0x00
    .quad 0x00
    .quad (0x80000 << 10) | 0xcf
    .fill 512-3, 8, 0



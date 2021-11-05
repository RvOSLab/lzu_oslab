    .globl boot_stack, boot_stack_top, _start, boot_pg_dir
    .section .text.entry

# 本项目所用代码模型为 medany, 该代码模型下编译器生成的代码以 PC 相对寻址的方式访问任意地址，地址通过 auipc 和 addi 指令获取。
# 可执行文件中的地址与加载后的内存地址相差 0x40000000，因此处理器访问到的地址加 0x40000000 才是可执行文件中符号的地址。

_start:
    la t0, boot_pg_dir
    srli t0, t0, 12
    li t1, (8 << 60)
    or t0, t0, t1
    csrw satp, t0
    sfence.vma

    li t1, 0x40000000
    la sp, boot_stack_top
    add sp, sp, t1
    la t0, main
    add t0, t0, t1
    jr t0

    .section .bss
boot_stack:
    .space 4096 * 8
boot_stack_top:

    .section .data
boot_pg_dir:
    .zero 2 * 8
    .quad (0x80000000 >> 2) | 0x0F
    .quad (0x80000000 >> 2) | 0x0F
    .zero 508 * 8

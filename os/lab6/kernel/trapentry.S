# 定义常量XLENB=8（每个寄存器 64 bit= 8 bytes)
.equ XLENB, 8

# 定义宏：LOAD，读取内存地址 sp+a2*8 的值到寄存器 a1
.macro LOAD a1, a2
    ld \a1, \a2*XLENB(sp)
.endm

# 定义宏：STORE，将寄存器 a1 的值保存到内存地址 sp+a2*8
.macro STORE a1, a2
    sd \a1, \a2*XLENB(sp)
.endm

# 定义宏：保存上下文（所有通用寄存器及额外的CSR寄存器）
# 我们规定：当 CPU 处于 U-Mode 时，sscratch 保存内核栈地址；处于 S-Mode 时，sscratch 为 0 。
.macro SAVE_ALL

    # 交换 sp 和 sscratch 寄存器
    csrrw sp, sscratch, sp

    # 可以通过sp是否为0判断原先是否为内核态
    # 如果中断来自用户态，此时 sp 已经指向内核栈，直接跳转到 trap_from_user 保存寄存器
    bnez sp, trap_from_user

# 否则为0，原本就是内核态，再次交换，继续往下执行，保存上下文
trap_from_kernel:

    # 将sscratch中的值读到sp中，此时 sscratch = 发生中断前的 sp（内核栈）
    csrr sp, sscratch

# 不为0，原本是用户态，不做交换，直接保存上下文
trap_from_user:
    # 将栈指针下移为TrapFrame预留足够的空间用于将所有通用寄存器值存入栈中（36个寄存器的空间）
    addi sp, sp, -36*XLENB
    # 保存所有通用寄存器，除了 x2 (x2就是sp，sp 本应保存的是发生中断前的值，这个值目前被交换到了 sscratch 中，因此留到后面处理。)
    STORE x1, 1
    STORE x3, 3
    STORE x4, 4
    STORE x5, 5
    STORE x6, 6
    STORE x7, 7
    STORE x8, 8
    STORE x9, 9
    STORE x10, 10
    STORE x11, 11
    STORE x12, 12
    STORE x13, 13
    STORE x14, 14
    STORE x15, 15
    STORE x16, 16
    STORE x17, 17
    STORE x18, 18
    STORE x19, 19
    STORE x20, 20
    STORE x21, 21
    STORE x22, 22
    STORE x23, 23
    STORE x24, 24
    STORE x25, 25
    STORE x26, 26
    STORE x27, 27
    STORE x28, 28
    STORE x29, 29
    STORE x30, 30
    STORE x31, 31

    # 保存完x0-x31之后这些寄存器就可以随意使用了，下面马上用到：
    # 读取sscratch到s0，赋 sscratch = 0（在内核态中，时刻保持sscratch为0，让s0存原有的sp）
    csrrw s0, sscratch, x0

    # 读取 sstatus, sepc, stval, scause寄存器的值到s0-s4（x1-x31中的特定几个）寄存器
    csrr s1, sstatus
    csrr s2, sepc
    csrr s3, stval
    csrr s4, scause

    # 存储 sp, sstatus, sepc, sbadvaddr, scause 到栈中
    # 其中把s0存到x2（sp）的位置以便于返回时直接恢复栈
    STORE s0, 2
    STORE s1, 32
    STORE s2, 33
    STORE s3, 34
    STORE s4, 35
.endm

# 定义宏：恢复寄存器
.macro RESTORE_ALL
    LOAD s1, 32             # s1 = sstatus
    LOAD s2, 33             # s2 = sepc
    andi s0, s1, 1 << 8     # 根据 sstatus.SPP 是否为 1 来判断中断前的特权级，1为内核态，0为用户态
    bnez s0, _to_kernel     # s0 = 是否会到内核态
# 若回到用户态
_to_user:
    addi s0, sp, 36*XLENB      # 计算出中断前的内核态sp，先存放在s0中
    csrw sscratch, s0         # 将s0中的内核态sp存入sscratch。根据规定，回到用户态后sscratch = 内核态sp
# 若回到内核态，跳过回到用户态的sscratch修改
_to_kernel:
    # 恢复 sstatus, sepc
    csrw sstatus, s1
    csrw sepc, s2

    # 恢复除了 x2 (sp) 以外的其余通用寄存器
    LOAD x1, 1
    LOAD x3, 3
    LOAD x4, 4
    LOAD x5, 5
    LOAD x6, 6
    LOAD x7, 7
    LOAD x8, 8
    LOAD x9, 9
    LOAD x10, 10
    LOAD x11, 11
    LOAD x12, 12
    LOAD x13, 13
    LOAD x14, 14
    LOAD x15, 15
    LOAD x16, 16
    LOAD x17, 17
    LOAD x18, 18
    LOAD x19, 19
    LOAD x20, 20
    LOAD x21, 21
    LOAD x22, 22
    LOAD x23, 23
    LOAD x24, 24
    LOAD x25, 25
    LOAD x26, 26
    LOAD x27, 27
    LOAD x28, 28
    LOAD x29, 29
    LOAD x30, 30
    LOAD x31, 31
    # 最后恢复栈指针(x2, sp)为原指针（无论是用户态还是内核态）
    LOAD x2, 2
.endm

# 代码段开始
.section .text
.globl __alltraps
.align 4
__alltraps:
    # 保存上下文
    SAVE_ALL
    # 将sp存入a0作为trap函数的第一个参数
    mv a0, sp
    # 跳转到trap函数执行（PC+4的值存入了x1寄存器），执行结束后会返回到这里继续向下执行恢复上下文
    call trap
.globl __trapret
__trapret:
    # 恢复上下文
    move sp, a0
    RESTORE_ALL
    # 从内核态中断中返回
    sret

.global __alltraps, __trapret
# 定义常量XLENB=8（每个寄存器 64 bit= 8 bytes)
XLENB = 8
SCAUSE = 8 * 35
UECALL = 8

# 定义宏：LOAD，读取内存地址 sp+a2*8 的值到寄存器 a1
.macro LOAD a1, a2
    ld \a1, \a2*XLENB(sp)
.endm

# 定义宏：STORE，将寄存器 a1 的值保存到内存地址 sp+a2*8
.macro STORE a1, a2
    sd \a1, \a2*XLENB(sp)
.endm

# 定义宏：保存上下文（所有通用寄存器及额外的CSR寄存器）
# sscratch 保存内核态堆栈指针
.macro SAVE_ALL

    # 交换 sp 和 sscratch 寄存器
    csrrw sp, sscratch, sp

    # 将栈指针下移为TrapFrame预留足够的空间用于将所有通用寄存器值存入栈中（36个寄存器的空间）
    addi sp, sp, -36*XLENB

    STORE x1, 1
    csrr  x1, sscratch
    csrw sscratch, sp      # 避免发生嵌套中断时 sp 指向用户态堆栈
    STORE x1, 2
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

    # 读取 sstatus, sepc, stval, scause寄存器的值到s0-s4（x1-x31中的特定几个）寄存器
    csrr s1, sstatus
    csrr s2, sepc
    csrr s3, stval
    csrr s4, scause

    # 存储 sp, sstatus, sepc, sbadvaddr, scause 到栈中
    # 其中把s0存到x2（sp）的位置以便于返回时直接恢复栈
    STORE s1, 32
    STORE s2, 33
    STORE s3, 34
    STORE s4, 35
.endm

# 定义宏：恢复寄存器
.macro RESTORE_ALL
    mv sp, a0
    addi a0, a0, 36 * XLENB
    csrw sscratch, a0

    # 恢复除了 x2 (sp) 以外的其余通用寄存器
    LOAD x1, 1
    LOAD x3, 3
    LOAD x3, 4
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

    LOAD x31, 32
    csrw sstatus, x31
    LOAD x31, 33
    csrw sepc, x31
    LOAD x31, 34
    csrw stval, x31
    LOAD x31, 35
    csrw scause, x31
    LOAD x31, 31

    # 最后恢复栈指针(x2, sp)为原指针（无论是用户态还是内核态）
    LOAD x2, 2
.endm

# 代码段
.section .text
__alltraps:
    # 保存上下文
    # 将sp存入a0作为trap函数的第一个参数
    SAVE_ALL
    mv a0, sp
    # 跳转到trap函数执行（PC+4的值存入了x1寄存器），执行结束后会返回到这里继续向下执行恢复上下文
    jal trap
__trapret:
    # 恢复上下文
    RESTORE_ALL
    # csrrs x0, sstatus, 0x00000002
    # 从内核态中断中返回
    sret


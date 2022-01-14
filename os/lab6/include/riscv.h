#ifndef __RISCV_H__
#define __RISCV_H__

/// @{ @name MIP 寄存器标志位
#define MIP_SSIP (1 << IRQ_S_SOFT)
#define MIP_HSIP (1 << IRQ_H_SOFT)
#define MIP_MSIP (1 << IRQ_M_SOFT)
#define MIP_STIP (1 << IRQ_S_TIMER)
#define MIP_HTIP (1 << IRQ_H_TIMER)
#define MIP_MTIP (1 << IRQ_M_TIMER)
#define MIP_SEIP (1 << IRQ_S_EXT)
#define MIP_HEIP (1 << IRQ_H_EXT)
#define MIP_MEIP (1 << IRQ_M_EXT)
/// @}

/// @{ @name 中断编号
#define IRQ_U_SOFT   0
#define IRQ_S_SOFT   1
#define IRQ_H_SOFT   2
#define IRQ_M_SOFT   3
#define IRQ_U_TIMER  4
#define IRQ_S_TIMER  5
#define IRQ_H_TIMER  6
#define IRQ_M_TIMER  7
#define IRQ_U_EXT    8
#define IRQ_S_EXT    9
#define IRQ_H_EXT   10
#define IRQ_M_EXT   11
#define IRQ_COP     12
#define IRQ_HOST    13
/// @}

/// @{ @name 异常编号
#define CAUSE_MISALIGNED_FETCH       0x0
#define CAUSE_FAULT_FETCH            0x1
#define CAUSE_ILLEGAL_INSTRUCTION    0x2
#define CAUSE_BREAKPOINT             0x3
#define CAUSE_MISALIGNED_LOAD        0x4
#define CAUSE_FAULT_LOAD             0x5
#define CAUSE_MISALIGNED_STORE       0x6
#define CAUSE_FAULT_STORE            0x7
#define CAUSE_USER_ECALL             0x8
#define CAUSE_SUPERVISOR_ECALL       0x9
#define CAUSE_HYPERVISOR_ECALL       0xa
#define CAUSE_MACHINE_ECALL          0xb
#define CAUSE_INSTRUCTION_PAGE_FAULT 0xc
#define CAUSE_LOAD_PAGE_FAULT        0xd
#define CAUSE_STORE_PAGE_FAULT       0xf
/// @}

/// @{ @name SSTATUS 寄存器标志位
#define SSTATUS_UIE          0x00000001
#define SSTATUS_SIE          0x00000002
#define SSTATUS_UPIE         0x00000010
#define SSTATUS_SPIE         0x00000020
#define SSTATUS_SPP          0x00000100
#define SSTATUS_FS           0x00006000
#define SSTATUS_XS           0x00018000
#define SSTATUS_PUM          0x00040000
#define SSTATUS32_SD         0x80000000
#define SSTATUS64_SD 0x8000000000000000
/// @}

/// @{ @name RISCV 权限模式
#define USER       0
#define SUPERVISOR 1
#define MACHINE    2
/// @}

#define INST_LEN(inst_ptr) (((*(char*)(inst_ptr)) & 0x3) < 0x3 ? 2 : 4)

/// @{ @name 操作控制状态寄存器（CSR）

/** 读取 CSR */
#define read_csr(reg)                                               \
    ({                                                              \
        uint64_t __tmp;                                        \
        asm volatile("csrr %0, " #reg : "=r"(__tmp));               \
        __tmp;                                                      \
    })

/** 读取寄存器 */
#define read_reg(reg)                                               \
    ({                                                              \
        uint64_t __tmp;                                        \
        asm volatile("mv %0, " #reg : "=r"(__tmp));               \
        __tmp;                                                      \
    })

/** 写 CSR */
#define write_csr(reg, val)                                         \
    ({                                                              \
        if (__builtin_constant_p(val) && (unsigned long)(val) < 32) \
            asm volatile("csrw " #reg ", %0" ::"i"(val));           \
        else                                                        \
            asm volatile("csrw " #reg ", %0" ::"r"(val));           \
    })


/** 写 CSR 并返回原值 */
#define swap_csr(reg, val)                                          \
    ({                                                              \
        unsigned long __tmp;                                        \
        if (__builtin_constant_p(val) && (unsigned long)(val) < 32) \
            asm volatile("csrrw %0, " #reg ", %1"                   \
                     : "=r"(__tmp)                                  \
                     : "i"(val));                                   \
        else                                                        \
            asm volatile("csrrw %0, " #reg ", %1"                   \
                     : "=r"(__tmp)                                  \
                     : "r"(val));                                   \
        __tmp;                                                      \
    })

/** 置位寄存器 */
#define set_csr(reg, bit)                                           \
    ({                                                              \
        unsigned long __tmp;                                        \
        if (__builtin_constant_p(bit) && (unsigned long)(bit) < 32) \
            asm volatile("csrrs %0, " #reg ", %1"                   \
                     : "=r"(__tmp)                                  \
                     : "i"(bit));                                   \
        else                                                        \
            asm volatile("csrrs %0, " #reg ", %1"                   \
                     : "=r"(__tmp)                                  \
                     : "r"(bit));                                   \
        __tmp;                                                      \
    })

/** 复位寄存器 */
#define clear_csr(reg, bit)                                         \
    ({                                                              \
        unsigned long __tmp;                                        \
        if (__builtin_constant_p(bit) && (unsigned long)(bit) < 32) \
            asm volatile("csrrc %0, " #reg ", %1"                   \
                     : "=r"(__tmp)                                  \
                     : "i"(bit));                                   \
        else                                                        \
            asm volatile("csrrc %0, " #reg ", %1"                   \
                     : "=r"(__tmp)                                  \
                     : "r"(bit));                                   \
        __tmp;                                                      \
    })

/// @}

#define disable_interrupt() clear_csr(sstatus, SSTATUS_SIE)
#define enable_interrupt() set_csr(sstatus, SSTATUS_SIE)

/**
 * @file riscv.h
 * @brief 操作 RISCV 寄存器
 */
#endif

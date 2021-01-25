#ifndef __RISCV_H__
#define __RISCV_H__

#define CSR_CYCLE 0xc00
#define CSR_TIME 0xc01
#define CSR_INSTRET 0xc02

/* 寄存器编号 */
#define CSR_SSTATUS 0x100
#define CSR_SIE 0x104
#define CSR_STVEC 0x105
#define CSR_SSCRATCH 0x140
#define CSR_SEPC 0x141
#define CSR_SCAUSE 0x142
#define CSR_SBADADDR 0x143
#define CSR_SIP 0x144
#define CSR_SPTBR 0x180
#define CSR_MSTATUS 0x300
#define CSR_MISA 0x301
#define CSR_MEDELEG 0x302
#define CSR_MIDELEG 0x303
#define CSR_MIE 0x304
#define CSR_MTVEC 0x305
#define CSR_MSCRATCH 0x340
#define CSR_MEPC 0x341
#define CSR_MCAUSE 0x342
#define CSR_MBADADDR 0x343
#define CSR_MIP 0x344
#define CSR_TSELECT 0x7a0
#define CSR_TDATA1 0x7a1
#define CSR_TDATA2 0x7a2
#define CSR_TDATA3 0x7a3
#define CSR_DCSR 0x7b0
#define CSR_DPC 0x7b1
#define CSR_DSCRATCH 0x7b2
#define CSR_MCYCLE 0xb00
#define CSR_MINSTRET 0xb02

#define MIP_SSIP (1 << IRQ_S_SOFT)
#define MIP_HSIP (1 << IRQ_H_SOFT)
#define MIP_MSIP (1 << IRQ_M_SOFT)
#define MIP_STIP (1 << IRQ_S_TIMER)
#define MIP_HTIP (1 << IRQ_H_TIMER)
#define MIP_MTIP (1 << IRQ_M_TIMER)
#define MIP_SEIP (1 << IRQ_S_EXT)
#define MIP_HEIP (1 << IRQ_H_EXT)
#define MIP_MEIP (1 << IRQ_M_EXT)

#define IRQ_U_SOFT 0
#define IRQ_S_SOFT 1
#define IRQ_H_SOFT 2
#define IRQ_M_SOFT 3
#define IRQ_U_TIMER 4
#define IRQ_S_TIMER 5
#define IRQ_H_TIMER 6
#define IRQ_M_TIMER 7
#define IRQ_U_EXT 8
#define IRQ_S_EXT 9
#define IRQ_H_EXT 10
#define IRQ_M_EXT 11
#define IRQ_COP 12
#define IRQ_HOST 13

#define CAUSE_MISALIGNED_FETCH 0x0
#define CAUSE_FAULT_FETCH 0x1
#define CAUSE_ILLEGAL_INSTRUCTION 0x2
#define CAUSE_BREAKPOINT 0x3
#define CAUSE_MISALIGNED_LOAD 0x4
#define CAUSE_FAULT_LOAD 0x5
#define CAUSE_MISALIGNED_STORE 0x6
#define CAUSE_FAULT_STORE 0x7
#define CAUSE_USER_ECALL 0x8
#define CAUSE_SUPERVISOR_ECALL 0x9
#define CAUSE_HYPERVISOR_ECALL 0xa
#define CAUSE_MACHINE_ECALL 0xb

#define SSTATUS_UIE 0x00000001
#define SSTATUS_SIE 0x00000002
#define SSTATUS_UPIE 0x00000010
#define SSTATUS_SPIE 0x00000020
#define SSTATUS_SPP 0x00000100
#define SSTATUS_FS 0x00006000
#define SSTATUS_XS 0x00018000
#define SSTATUS_PUM 0x00040000
#define SSTATUS32_SD 0x80000000
#define SSTATUS64_SD 0x8000000000000000

#define read_csr(reg)                                                          \
	({                                                                     \
		unsigned long __tmp;                                           \
		asm volatile("csrr %0, " #reg : "=r"(__tmp));                  \
		__tmp;                                                         \
	})

#define write_csr(reg, val)                                                    \
	({                                                                     \
		if (__builtin_constant_p(val) && (unsigned long)(val) < 32)    \
			asm volatile("csrw " #reg ", %0" ::"i"(val));          \
		else                                                           \
			asm volatile("csrw " #reg ", %0" ::"r"(val));          \
	})

#define swap_csr(reg, val)                                                     \
	({                                                                     \
		unsigned long __tmp;                                           \
		if (__builtin_constant_p(val) && (unsigned long)(val) < 32)    \
			asm volatile("csrrw %0, " #reg ", %1"                  \
				     : "=r"(__tmp)                             \
				     : "i"(val));                              \
		else                                                           \
			asm volatile("csrrw %0, " #reg ", %1"                  \
				     : "=r"(__tmp)                             \
				     : "r"(val));                              \
		__tmp;                                                         \
	})

#define set_csr(reg, bit)                                                      \
	({                                                                     \
		unsigned long __tmp;                                           \
		if (__builtin_constant_p(bit) && (unsigned long)(bit) < 32)    \
			asm volatile("csrrs %0, " #reg ", %1"                  \
				     : "=r"(__tmp)                             \
				     : "i"(bit));                              \
		else                                                           \
			asm volatile("csrrs %0, " #reg ", %1"                  \
				     : "=r"(__tmp)                             \
				     : "r"(bit));                              \
		__tmp;                                                         \
	})

#define clear_csr(reg, bit)                                                    \
	({                                                                     \
		unsigned long __tmp;                                           \
		if (__builtin_constant_p(bit) && (unsigned long)(bit) < 32)    \
			asm volatile("csrrc %0, " #reg ", %1"                  \
				     : "=r"(__tmp)                             \
				     : "i"(bit));                              \
		else                                                           \
			asm volatile("csrrc %0, " #reg ", %1"                  \
				     : "=r"(__tmp)                             \
				     : "r"(bit));                              \
		__tmp;                                                         \
	})

#define rdtime() read_csr(CSR_TIME)
#define rdcycle() read_csr(CSR_CYCLE)
#define rdinstret() read_csr(CSR_INSTRET)

#endif
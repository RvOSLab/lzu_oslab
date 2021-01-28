/**
 * @file trap.c
 * @author Hanabichan (93yutf@gmail.com)
 * @brief 定义了中断相关的常量、变量与函数
 *
 * 发生中断时CPU会干的事情：
 * 发生例外的指令的 PC 被存入 sepc 用于恢复，且 PC 被设置为 stvec（存了中断处理程序的地址）。
 * scause 根据异常类型设置，stval 被设置成出错的地址或者其它特定异常的信息字。
 * 把 sstatus CSR 中的 SIE 置零，屏蔽中断，且 SIE 之前的值被保存在 SPIE 中。
 * 发生例外时的权限模式被保存在 sstatus 的 SPP 域，然后设置当前模式为 S 模式。
 */

#include <riscv.h>
#include <trap.h>
#include <clock.h>
#include <sbi.h>
#include <kdebug.h>

/** 时间片长度 */
#define PLANED_TICK_NUM 100

static inline void trap_dispatch(struct trapframe *tf);
static void interrupt_handler(struct trapframe *tf);
static void exception_handler(struct trapframe *tf);

/**
 * @brief 初始化中断
 * 设置 STVEC（中断向量表）的值为 __alltraps 的地址
 *
 * 在 SSTATUS 中启用 interrupt
 * 注：下面的CSR操作均为宏定义，寄存器名直接以字符串形式传递，并没有相应的变量
 */
void trap_init()
{
	/* 引入 trapentry.s 中定义的外部函数，便于下面取地址 */
	extern void __alltraps(void);
	/* 
     * 设置 sscratch 寄存器为 0, 表示我们正在内核中执行
     * 规定当 CPU 处于 U-Mode 时，sscratch 保存内核栈地址；处于 S-Mode 时，sscratch 为 0 。具体看文档
     */
	write_csr(sscratch, 0);
	/* 设置STVEC的值，MODE=00，因为地址的最后两位四字节对齐后必为0，因此不用单独设置MODE */
	write_csr(stvec, &__alltraps);
    /* 启用 interrupt，sstatus的SSTATUS_SIE位置1 */
	set_csr(sstatus, SSTATUS_SIE);
}

/**
 * @brief 中断处理函数，从 trapentry.s 跳转而来
 * @param tf 中断保存栈
 */
void trap(struct trapframe *tf)
{
	trap_dispatch(tf);
}

/**
 * @brief 中断类型的检查与分派
 * @param tf 中断保存栈
 */
static inline void trap_dispatch(struct trapframe *tf)
{
	/* interrupts，中断，scause最高位为1 */
	if ((int64_t)tf->cause < 0) {
		interrupt_handler(tf);
	} else
	/* exceptions，异常，scause最高位为0 */
	{
		exception_handler(tf);
	}
}

/**
 * @brief 中断处理函数，将不同种类中断分配给不同分支
 * @param tf 中断保存栈
 */
void interrupt_handler(struct trapframe *tf)
{
	/** 置cause的最高位为0 */
	int64_t cause = (tf->cause << 1) >> 1;
	switch (cause) {
	case IRQ_U_SOFT:
		kputs("User software interrupt\n");
		break;
	case IRQ_S_SOFT:
		kputs("Supervisor software interrupt\n");
		break;
	case IRQ_H_SOFT:
		kputs("Hypervisor software interrupt\n");
		break;
	case IRQ_M_SOFT:
		kputs("Machine software interrupt\n");
		break;
	case IRQ_U_TIMER:
		kputs("User timer interrupt\n");
		break;
	case IRQ_S_TIMER:
		clock_set_next_event();
		if (++ticks % PLANED_TICK_NUM == 0) {
			kprintf("%u ticks\n", ticks);
		}
		break;
	case IRQ_H_TIMER:
		kputs("Hypervisor timer interrupt\n");
		break;
	case IRQ_M_TIMER:
		kputs("Machine timer interrupt\n");
		break;
	case IRQ_U_EXT:
		kputs("User external interrupt\n");
		break;
	case IRQ_S_EXT:
		kputs("Supervisor external interrupt\n");
		break;
	case IRQ_H_EXT:
		kputs("Hypervisor external interrupt\n");
		break;
	case IRQ_M_EXT:
		kputs("Machine external interrupt\n");
		break;
	default:
		print_trapframe(tf);
		break;
	}
}

/**
 * @brief 异常处理函数，将不同种类中断分配给不同分支
 * @param tf 中断保存栈
 */
void exception_handler(struct trapframe *tf)
{
	switch (tf->cause) {
	case CAUSE_MISALIGNED_FETCH:
		kputs("misaligned fetch");
		break;
	case CAUSE_FAULT_FETCH:
		kputs("fault fetch");
		break;
	case CAUSE_ILLEGAL_INSTRUCTION:
		kputs("illegal instruction");
		break;
	case CAUSE_BREAKPOINT:
		kputs("breakpoint");
		print_trapframe(tf);
		tf->epc += 2;
		break;
	case CAUSE_MISALIGNED_LOAD:
		kputs("misaligned load");
		break;
	case CAUSE_FAULT_LOAD:
		kputs("fault load");
		break;
	case CAUSE_MISALIGNED_STORE:
		kputs("misaligned store");
		break;
	case CAUSE_FAULT_STORE:
		kputs("fault store");
		break;
	case CAUSE_USER_ECALL:
		kputs("user_ecall");
		break;
	case CAUSE_SUPERVISOR_ECALL:
		kputs("supervisor_ecall");
		break;
	case CAUSE_HYPERVISOR_ECALL:
		kputs("hypervisor_ecall");
		break;
	case CAUSE_MACHINE_ECALL:
		kputs("machine_ecall");
		break;
	default:
		print_trapframe(tf);
		break;
	}
}

/**
 * @brief 打印中断保存栈
 * @param tf 中断保存栈
 */
void print_trapframe(struct trapframe *tf)
{
	kprintf("trapframe at %p\n\n", tf);
	print_regs(&tf->gpr);
	kprintf("  trap information:\n");
	kprintf("  status   0x%x\n", tf->status);
	kprintf("  epc      0x%x\n", tf->epc);
	kprintf("  badvaddr 0x%x\n", tf->badvaddr);
	kprintf("  cause    0x%x\n", tf->cause);
}

/**
 * @brief 打印中断保存栈中的寄存器信息部分
 * @param gpr 中断保存栈中的寄存器信息部分
 */
void print_regs(struct pushregs *gpr)
{
	kprintf("  registers:\n");
	kprintf("  zero     0x%x\n", gpr->zero);
	kprintf("  ra       0x%x\n", gpr->ra);
	kprintf("  sp       0x%x\n", gpr->sp);
	kprintf("  gp       0x%x\n", gpr->gp);
	kprintf("  tp       0x%x\n", gpr->tp);
	kprintf("  t0       0x%x\n", gpr->t0);
	kprintf("  t1       0x%x\n", gpr->t1);
	kprintf("  t2       0x%x\n", gpr->t2);
	kprintf("  s0       0x%x\n", gpr->s0);
	kprintf("  s1       0x%x\n", gpr->s1);
	kprintf("  a0       0x%x\n", gpr->a0);
	kprintf("  a1       0x%x\n", gpr->a1);
	kprintf("  a2       0x%x\n", gpr->a2);
	kprintf("  a3       0x%x\n", gpr->a3);
	kprintf("  a4       0x%x\n", gpr->a4);
	kprintf("  a5       0x%x\n", gpr->a5);
	kprintf("  a6       0x%x\n", gpr->a6);
	kprintf("  a7       0x%x\n", gpr->a7);
	kprintf("  s2       0x%x\n", gpr->s2);
	kprintf("  s3       0x%x\n", gpr->s3);
	kprintf("  s4       0x%x\n", gpr->s4);
	kprintf("  s5       0x%x\n", gpr->s5);
	kprintf("  s6       0x%x\n", gpr->s6);
	kprintf("  s7       0x%x\n", gpr->s7);
	kprintf("  s8       0x%x\n", gpr->s8);
	kprintf("  s9       0x%x\n", gpr->s9);
	kprintf("  s10      0x%x\n", gpr->s10);
	kprintf("  s11      0x%x\n", gpr->s11);
	kprintf("  t3       0x%x\n", gpr->t3);
	kprintf("  t4       0x%x\n", gpr->t4);
	kprintf("  t5       0x%x\n", gpr->t5);
	kprintf("  t6       0x%x\n\n", gpr->t6);
}

/**
 * @brief 检测中断是否发生在内核态
 * @param tf 中断保存栈
 * @return int 1为内核态，0为用户态
 */
int trap_in_kernel(struct trapframe *tf)
{
	/* 根据 sstatus.SPP（sstatus的右数第9位）是否为 1 来判断中断前的特权级，1为内核态，0为用户态 */
	return (tf->status & SSTATUS_SPP) != 0;
}

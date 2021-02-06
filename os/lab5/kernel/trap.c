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
#include <assert.h>
#include <errno.h>
#include <sched.h>
#include <syscall.h>

static inline struct trapframe *trap_dispatch(struct trapframe *tf);
static struct trapframe *interrupt_handler(struct trapframe *tf);
static struct trapframe *exception_handler(struct trapframe *tf);
static struct trapframe *syscall_handler(struct trapframe *tf);
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
    /* sscratch 保存内核栈地址 */
	write_csr(sscratch, (char*)&init_task + PAGE_SIZE);
	/* 设置STVEC的值，MODE=00，因为地址的最后两位四字节对齐后必为0，因此不用单独设置MODE */
	write_csr(stvec, &__alltraps);
	/* 启用 interrupt，sstatus的SSTATUS_SIE位置1 */
	set_csr(sstatus, SSTATUS_SIE);
}

/**
 * @brief 中断处理函数，从 trapentry.s 跳转而来
 * @param tf 中断保存栈
 */
struct trapframe *trap(struct trapframe *tf)
{
	return trap_dispatch(tf);
}

/**
 * @brief 中断类型的检查与分派
 * @param tf 中断保存栈
 * @todo 清理注释
 */
static inline struct trapframe *trap_dispatch(struct trapframe *tf)
{
	return ((int64_t)tf->cause < 0) ? interrupt_handler(tf) :
						exception_handler(tf);
}

/**
 * @brief 中断处理函数，将不同种类中断分配给不同分支
 * @param tf 中断保存栈
 */
struct trapframe *interrupt_handler(struct trapframe *tf)
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
	case IRQ_S_TIMER:
		clock_set_next_event();
		/* Note:
         * 1. 用户进程只能通过系统调用、中断和异常进入内核态，
         *    这时系统自动关闭中断（禁止嵌套中断），用户进程不可能在内核态发生中断。
         *    在内核态发生中断只可能是内核或进程 0。
         *    因此，可以认为现在的代码无法正确计算进程在内核态下的运行时间。
         * 2. 禁止嵌套中断是否会导致发生系统调用时，系统的中断丢失，比如无法接受键盘
         *    输入、无法收到 IO 完成的中断。
         */
        if (!current) {
            current = (struct task_struct *)&init_task;
            /* INCOMPLETE!!! 暂时不切换到应用态 */
            /* tf->status &= ~SSTATUS_SPP [> 0x00000100 <]; */
        }

		if (trap_in_kernel(tf)) {
			++current->cstime;
		} else {
			++current->cutime;
		}

        /* Note:
         * 1. task0 的 counter 被设置为 1 以便在第一次时钟中断中切换到 task0。
         *    之后发生溢出，如果任意时刻又切换到 task0,意味着永远不发生进程切换。
         * 2. UNIX 系统在内核态不发生进程切换（时钟中断导致的），这里破坏了这一
         *    原则
         */
        if (!--current->counter) {
            return switch_to(tf, schedule());
        }
		break;
	case IRQ_H_TIMER:
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
    return tf;
}

/**
 * @brief 异常处理函数，将不同种类中断分配给不同分支
 * @param tf 中断保存栈
 */
struct trapframe * exception_handler(struct trapframe *tf)
{
	switch (tf->cause) {
	case CAUSE_MISALIGNED_FETCH:
		kputs("misaligned fetch");
		print_trapframe(tf);
        sbi_shutdown();
		break;
	case CAUSE_FAULT_FETCH:
		kputs("fault fetch");
		print_trapframe(tf);
        sbi_shutdown();
		break;
	case CAUSE_ILLEGAL_INSTRUCTION:
		panic("illegal instruction: %p", tf->badvaddr);
		break;
	case CAUSE_BREAKPOINT:
		kputs("breakpoint");
		tf->epc += 2;
		break;
	case CAUSE_MISALIGNED_LOAD:
		kputs("misaligned load");
        print_trapframe(tf);
        sbi_shutdown();
		break;
	case CAUSE_FAULT_LOAD:
		kputs("fault load");
        print_trapframe(tf);
        sbi_shutdown();
		break;
	case CAUSE_MISALIGNED_STORE:
		kputs("misaligned store");
        print_trapframe(tf);
        sbi_shutdown();
		break;
	case CAUSE_FAULT_STORE:
		kputs("fault store");
        print_trapframe(tf);
        sbi_shutdown();
		break;
	case CAUSE_USER_ECALL:
        return syscall_handler(tf);
		break;
	case CAUSE_SUPERVISOR_ECALL:
        /* if (tf->gpr.a7 != NR_fork) { */
            kputs("supervisor ecall");
            print_trapframe(tf);
            sbi_shutdown();
        /* } */
        /* return syscall_handler(tf); */
		break;
	case CAUSE_MACHINE_ECALL:
		kputs("machine ecall");
        print_trapframe(tf);
        sbi_shutdown();
		break;
    case CAUSE_INSTRUCTION_PAGE_FAULT:
        kputs("instruction page fault");
        print_trapframe(tf);
        sbi_shutdown();
        break;
    case CAUSE_LOAD_PAGE_FAULT:
        kputs("load page fault");
        print_trapframe(tf);
        sbi_shutdown();
        break;
    case CAUSE_STORE_PAGE_FAULT:
        kputs("store page fault");
        print_trapframe(tf);
        sbi_shutdown();
	default:
        kputs("unknown exception");
        print_trapframe(tf);
        sbi_shutdown();
		break;
	}
    return tf;
}

static struct trapframe *syscall_handler(struct trapframe *tf)
{
    uint64_t syscall_nr = tf->gpr.a7;
    if (syscall_nr >= NR_syscalls) {
        tf->gpr.a0 = -1;
        errno = ENOSYS;
    } else {
        tf->gpr.a0 = syscall_table[syscall_nr](tf);
    }
    /* tf->epc += 4; */
    return tf;
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

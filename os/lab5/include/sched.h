#ifndef __SCHED_H__
#define __SCHED_H__
#include <mm.h>
#include <trap.h>
#include <riscv.h>

#define NR_TASKS             512                              /**< 系统最大进程数 */

#define LAST_TASK tasks[NR_TASKS - 1]                         /**< tasks[] 数组最后一项 */
#define FIRST_TASK tasks[0]                                   /**< tasks[] 数组第一项 */

/// @{ @name 进程状态
#define TASK_RUNNING         0                                /**< 正在运行 */
#define TASK_INTERRUPTIBLE   1                                /**< 可中断阻塞 */
#define TASK_UNINTERRUPTIBLE 2                                /**< 不可中断阻塞 */
#define TASK_ZOMBIE          3                                /**< 僵尸状态（进程已终止，但未被父进程回收）*/
#define TASK_STOPPED         4                                /**< 进程停止 */
/// @}

typedef struct trapframe context;                             /**< 处理器上下文 */

/** 进程控制块 PCB(Process Control Block) */
struct task_struct {
	uint32_t state;                                           /**< 进程调度状态 */
	uint32_t counter;                                         /**< 时间片大小 */
	uint32_t priority;                                        /**< 进程优先级 */
	struct task_struct	*p_pptr, *p_cptr, *p_ysptr, *p_osptr; /**< 父、子、兄、弟进程 */
	uint32_t timeout;
	uint32_t utime,stime;                                     /**< 用户态、内核态耗时 */
    uint32_t cutime,cstime;                                   /**< 进程及其子进程内核、用户态总耗时 */
    uint32_t start_time;                                      /**< 进程创建的时间 */
    context context;                                          /**< 处理器状态 */
    uint64_t *pg_dir;                                         /**< 页目录地址 */
};


/**
 * @brief 初始化进程 0
 * 手动触发中断，调用 sys_init() 初始化进程0
 * @see sys_init()
 */
#define init_task0()                                                 \
({                                                                   \
    __label__ ret;                                                   \
    write_csr(scause, CAUSE_USER_ECALL);                             \
    set_csr(sstatus, SSTATUS_SPP);                                   \
    set_csr(sstatus, SSTATUS_SPIE);                                  \
    write_csr(sepc, &&ret - 4);                                      \
    register uint64_t a7 asm("a7") = 0;                              \
    __asm__ __volatile__("call __alltraps \n\t" ::"r"(a7):"memory"); \
    ret: ;                                                           \
})

/**
 * @brief 进程数据结构占用的页
 *
 * 进程 PCB 和内核态堆栈共用一页。
 * PCB 处于一页的低地址，内核态堆栈从页最高地址到低地址增长。
 */
union task_union {
    struct task_struct task;                                  /**< 进程控制块 */
    char stack[PAGE_SIZE];                                    /**< 内核态堆栈 */
};

extern struct task_struct *current;
extern struct task_struct *tasks[NR_TASKS];
extern union task_union init_task;

void sched_init();
size_t schedule();
void save_context(context *context);
context* push_context(char *stack, context *context);
context * switch_to(context *context, size_t task);
/**
 * @file sched.h
 * @brief 声明进程相关的数据类型和宏
 */
#endif /* end of include guard: __SCHED_H__ */

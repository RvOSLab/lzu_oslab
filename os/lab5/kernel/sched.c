#include <assert.h>
#include <sched.h>
#include <clock.h>
#include <kdebug.h>
#include <trap.h>
#include <riscv.h>
#include <sched.h>
#include <mm.h>

extern char *boot_stack_top;
extern uint64_t boot_pg_dir[512];
/** 进程 0 */
union task_union init_task;

/** 当前进程进程控制块 */
struct task_struct *current = NULL;

/** 系统所有进程的进程控制块指针数组 */
struct task_struct *tasks[NR_TASKS];

/**
 * @brief 将进程处理器状态 context 压入进程内核堆栈
 *
 * @param stack 内核堆栈指针
 * @param context 进程处理器状态
 * @return 堆栈上的 context 结构体指针
 */
context *push_context(char *stack, context *context)
{
	char *new_stack = stack - sizeof(struct trapframe);
	*(struct trapframe *)new_stack = *context;
	return (struct trapframe *)new_stack;
}

/**
 * @brief 保存当前进程的处理器状态
 *
 * 只在中断中保存处理器状态，因此保存的是中断发生后的状态
 * @param context 处理器状态指针
 */
void save_context(context *context)
{
	current->context = *context;
}

/**
 * @brief 初始化进程模块
 *
 * 主要负责初始化进程 0
 */
void sched_init()
{
	init_task.task = (struct task_struct){
		.state = TASK_RUNNING,
		.counter = 1,     /* 确保创建 task 0 后第一次时钟中断进行调度 */
		.priority = 15,
		.p_pptr = NULL,
		.p_cptr = NULL,
		.p_ysptr = NULL,
		.p_osptr = NULL,
		.timeout = 0,
		.utime = 0,
		.stime = 0,
		.cutime = 0,
		.cstime = 0,
	};
	init_task.task.start_time = ticks /* 0 */;
    tasks[0] = (struct task_struct *)&init_task;
	for (size_t i = 1; i < NR_TASKS; ++i) {
		tasks[i] = NULL;
	}
    pg_dir = boot_pg_dir;
    memset(pg_dir, 0, PAGE_SIZE);
	map_kernel(KERN_RWX);    /* INCOMPLETE!!!: 暂时不切换到应用态 */
	init_task.task.pg_dir = pg_dir;
    active_mapping();        /* 激活映射 */
}

/* 可以处理切换到当前进程的情况 */
/**
 * @brief 切换进程
 *
 * 保存当前进程处理器状态 `context`，切换到进程号为 `task` 的进程
 *
 * @return 目标进程的处理器状态指针
 * @note 本函数可以正确处理切换到当前进程的情况
 */
context *switch_to(context *context, size_t task)
{
	if (current == tasks[task]) {
		return context;
	}

	current = tasks[task];
	pg_dir = current->pg_dir;
	active_mapping();
	save_context(context);
	char *stack = (char *)current + PAGE_SIZE;
	return push_context(stack, &current->context);
}

/**
 * @brief 进程调度函数
 *
 * 使用公平调度算法。
 * 进程 0 不参加调度，当且仅当只有进程 0 时选择进程 0
 *
 * @return 目标进程的进程号
 */
size_t schedule()
{
	int i, next, c;
	struct task_struct **p;

	while (1) {
		c = -1;
		next = 0;
		i = NR_TASKS;
		p = &tasks[NR_TASKS];
		while (--i) {
			if (!*--p)
				continue;
			if ((*p)->state == TASK_RUNNING &&
			    (*p)->counter >
				    c) { /* (*p)->counter 一定大于等于零 */
				c = (*p)->counter;
				next = i;
			}
		}

		/* 没有用户进程 */
		if (c)
			break;

		/* 所有可运行的进程都耗尽了时间片 */
		for (p = &LAST_TASK; p > &FIRST_TASK; --p) {
			if (*p) {
				(*p)->counter =
					((*p)->counter >> 1) + (*p)->priority;
			}
		}
	}
	return next;
}

/**
 * @brief 系统调用 fork()
 * @todo 完成 sys_fork()
 */
int sys_fork(struct trapframe *tf)
{
    /* TODO */
    return 0;
}

/**
 * @file sched.c
 * @brief 实现绝大部分和进程有关的函数
 */

/**
 * @file sched.c
 * @brief 实现绝大部分和进程有关的函数
 */
#include <assert.h>
#include <clock.h>
#include <errno.h>
#include <kdebug.h>
#include <mm.h>
#include <riscv.h>
#include <sched.h>
#include <string.h>
#include <trap.h>
extern void boot_stack_top(void); /** 启动阶段内核堆栈最高地址处 */

/** 进程 0 */
union task_union init_task;

/** 当前进程进程控制块 */
struct task_struct* current = NULL;

/** 系统所有进程的进程控制块指针数组 */
struct task_struct* tasks[NR_TASKS];

/**
 * @brief 将进程处理器状态 context 压入进程内核堆栈
 *
 * @param stack 内核堆栈指针
 * @param context 进程处理器状态
 * @return 堆栈上的 context 结构体指针
 */
context* push_context(char* stack, context* context)
{
    char* new_stack = stack - sizeof(struct trapframe);
    *(struct trapframe*)new_stack = *context;
    return (struct trapframe*)new_stack;
}

/**
 * @brief 保存当前进程的处理器状态
 *
 * 只在中断中保存处理器状态，因此保存的是中断发生后的状态
 * @param context 处理器状态指针
 */
void save_context(context* context)
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
    tasks[0] = (struct task_struct*)&init_task;
    for (size_t i = 1; i < NR_TASKS; ++i) {
        tasks[i] = NULL;
    }
    init_task.task = (struct task_struct) {
        .state = TASK_RUNNING,
        .counter = 15,
        .priority = 15,
        .start_code = START_CODE,
        .start_data = START_DATA,
        .start_stack = START_STACK,
        .start_kernel = START_KERNEL,
        .start_time = ticks /* 0 */,
        .pg_dir = pg_dir,
    };

    current = &init_task.task;
}

/**
 * @brief 切换进程
 *
 * 保存当前进程处理器状态 `context`，切换到进程号为 `task` 的进程
 *
 * @return 目标进程的处理器状态指针
 * @note 本函数可以正确处理切换到当前进程的情况
 */
context* switch_to(context* context, size_t task)
{
    if (current == tasks[task]) {
        return context;
    }

    save_context(context);
    current = tasks[task];
    pg_dir = current->pg_dir;
    active_mapping();
    char* stack = (char*)current + PAGE_SIZE;
    return push_context(stack, &current->context);
}

/**
 * @brief 进程调度函数
 *
 * 使用公平调度算法(CFS)。
 * 进程 0 不参加调度，当且仅当只有进程 0 时选择进程 0
 *
 * @return 目标进程的进程号
 */
size_t schedule()
{
    int i, next, c;
    struct task_struct** p;

    while (1) {
        c = -1;
        next = 0;
        i = NR_TASKS;
        p = &tasks[NR_TASKS];
        while (--i) {
            if (!*--p)
                continue;
            /* 小心混用无符号数和有符号数！ */
            if ((*p)->state == TASK_RUNNING && (int32_t)(*p)->counter > c) {
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
                (*p)->counter = ((*p)->counter >> 1) + (*p)->priority;
            }
        }
    }
    return next;
}

/**
 * @brief 创建进程 0 各段的映射
 *
 * @param start 虚拟地址起始
 * @param end 虚拟地址结束
 * @param flag 权限位
 */
static void map_segment(uint64_t start, uint64_t end, uint8_t flag)
{
    uint64_t physical = start - LINEAR_OFFSET;
    start -= SBI_END + LINEAR_OFFSET - START_CODE;
    end -= SBI_END + LINEAR_OFFSET - START_CODE;
    while (start != end) {
        put_page(physical, start, flag);
        start += PAGE_SIZE;
        ++mem_map[MAP_NR(physical)];
        physical += PAGE_SIZE;
    }
}

/**
 * @brief 初始化进程 0
 *
 * 创建用户态进程零，将内核移入进程中。
 * @see init_task0()
 * @note 只有在发生中断时才能保存处理器状态，
 *       因此让 sys_init() 占据系统调用号 0,
 *       但是实际上 sys_init() 被内核调用，并
 *       不是系统调用。
 */
long sys_init(struct trapframe* tf)
{
    /* 创建指向内核代码/数据的用户态映射 */
    map_segment((uint64_t)text_start, (uint64_t)rodata_start, USER_RX | PAGE_PRESENT);
    map_segment((uint64_t)rodata_start, (uint64_t)data_start, USER_R | PAGE_PRESENT);
    map_segment((uint64_t)data_start, (uint64_t)kernel_end, USER_RW | PAGE_PRESENT);

    /*
     * 创建进程 0 堆栈（从 0xBFFFFFF0 开始）
     *
     * 内核堆栈已使用的空间远小于一页，因此仅为用户态堆栈分配一页虚拟页。
     * 为了确保切换到应用态后正确执行，将内核态堆栈的数据全部拷贝到用用户态堆栈中。
     */
    get_empty_page(START_STACK - PAGE_SIZE, USER_RW);
    get_empty_page(START_STACK, USER_RW);
    invalidate();
    memcpy((void*)((uint64_t)START_STACK - PAGE_SIZE), (const void*)FLOOR(tf->gpr.sp), PAGE_SIZE);
    tf->gpr.sp = START_STACK - ((uint64_t)boot_stack_top - tf->gpr.sp);
    /* GCC 使用 s0 指向函数栈帧起始地址（高地址），因此这里也要修改，否则切换到进程0会访问到内核区 */
    tf->gpr.s0 = START_STACK;
    save_context(tf);
    return 0;
}

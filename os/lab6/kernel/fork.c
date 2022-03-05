/**
 * @file fork.c
 * @brief 实现系统调用 fork()
 *
 * fork() 的原型为`pid_t fork(void)`，返回值（PID）不一定是整数。
 * 这里将 PID 实现为整数。
 */
#include <errno.h>
#include <sched.h>
#include <clock.h>
#include <mm.h>
#include <string.h>

/**
 * @brief 将当前进程的虚拟地址空间拷贝给进程 p
 *
 * @param p task_struct 指针
 */
int copy_mem(struct task_struct * p)
{
    copy_page_tables(0, p->pg_dir, 0, current->start_kernel);
    copy_page_tables(current->start_kernel, p->pg_dir, p->start_kernel, 0x100000000 - current->start_kernel);
    return 1;
}

/**
 * @brief 取消从 addr 开始 size 字节的写保护
 *
 * @param addr 起始虚拟地址
 * @param size 字节数
 */
void verify_area(uint64_t addr,int size)
{
    size += addr & 0xFFF;
    addr &= ~0x00000FFF;
    while (size>0) {
        size -= 4096;
        write_verify(addr);
        addr += 4096;
    }
}
/**
 * @brief 获取可用的 PID
 *
 * 返回的子进程 PID 不等于任何进程的 PID 和 PGID（进程组 ID）。
 * PID 被实现为`tasks[]`数组下标。
 *
 * @return 返回可用的 PID;无可用 PID 则返回 NR_TASKS。
 */
static uint32_t find_empty_process()
{
    uint32_t pid = 0;
    size_t i;
    while (++pid < NR_TASKS) {
        for (i = 0; i < NR_TASKS; ++i) {
            if (tasks[i] && (pid == i || pid == tasks[i]->pgid)) {
                break;
            }
        }
        if (i == NR_TASKS) {
            break;
        }
    }
    return pid;
}

/**
 * @brief 实现系统调用 fork()
 */
long sys_fork(struct trapframe *tf)
{
    uint32_t nr = find_empty_process();
    if (nr == NR_TASKS) {
        return -EAGAIN;
    }
    uint64_t page = get_free_page();
    if (!page) {
        return -EAGAIN;
    }
    struct task_struct* p = (struct task_struct *)VIRTUAL(page);

    uint64_t page_dir = get_free_page();
    if (!page_dir) {
        free_page(page);
        return -EAGAIN;
    }

    memcpy(p, current, sizeof(struct task_struct) - sizeof(struct trapframe));
    p->context = *tf;
    p->context.epc += INST_LEN(p->context.epc);
    p->pg_dir = (uint64_t *)VIRTUAL(page_dir);
    tasks[nr] = p;
    kprintf("process %x forks process %x\n", (uint64_t)current->pid, (uint64_t)nr);

    /* 在此之间发生错误，将不会创建进程，系统处于安全状态 */
    copy_mem(p);

    p->state = TASK_UNINTERRUPTIBLE;
    p->pid = nr;
    p->counter = p->priority = 15;
    p->start_time = ticks;
    p->p_pptr = current;
    p->p_osptr = current->p_ysptr;
    if (p->p_osptr) {
        p->p_osptr->p_ysptr = current;
    }
    current->p_cptr = p;
    p->context.gpr.a0 = 0; /* 新进程 fork() 返回值 */
    p->state= TASK_RUNNING;
    return nr;
}

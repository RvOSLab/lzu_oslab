/**
 * @file syscall.c
 * @brief 定义系统调用表，实现大部分系统调用
 *
 * 所有系统调用都由`sys_xxx()`实现，`sys_xxx()`以中断堆栈指针为
 * 参数。系统调用具体的参数类型和大小从堆栈指针中获取。通过`syscall(nr, ...)`
 * 调用系统调用。
 *
 */
#include <trap.h>
#include <kdebug.h>
#include <syscall.h>
#include <assert.h>
#include <stdarg.h>
#include <errno.h>
#include <sched.h>
#include <device.h>
#include <fs/vfs.h>
#include <lib/sleep.h>

extern long sys_init(struct trapframe *);
extern long sys_fork(struct trapframe *);

/**
 * @brief 测试 fork() 是否正常工作
 *
 * 如果`fork()`和虚拟内存正常工作，将实现写时复制，
 * 不同进程的同一局部变量可以被写入不同的值。
 * 暂时还未实现字符 IO，因此只能在内核态下以这种办法
 * 检测`fork()`和写时复制是否正常。
 *
 * @param 参数1 - 局部变量（8 字节）
 */
static long sys_test_fork(struct trapframe *tf)
{
    kprintf("process %u: local - %u\n",
            (uint64_t)current->pid,
            tf->gpr.a0);
    return 0;
}

/**
 * @brief 获取当前进程 PID
 */
static long sys_getpid(struct trapframe *tf)
{
    return current->pid;
}

/**
 * @brief 获取当前进程父进程 ID
 */
static long sys_getppid(struct trapframe *tf)
{
    if (current == tasks[0]) {
        return 0;
    } else {
        return current->p_pptr->pid;
    }
}

/**
 * @brief 获取一个字符或输出一个字符
 */
static long sys_char(struct trapframe *tf)
{
    return char_dev_test(tf->gpr.a0);
}

/**
 * @brief 块设备测试
 */
static long sys_block(struct trapframe *tf)
{
    return block_dev_test();
}

/**
 * @brief open
 */
static long sys_open(struct trapframe *tf)
{
    uint64_t fd = 0;
    while (fd < 4) {
        if (!current->fd[fd]) {
            struct vfs_inode *inode = vfs_get_inode((const char *)tf->gpr.a0, NULL);
            current->fd[fd] = inode;
            if (inode) {
                vfs_ref_inode(inode);
                return fd;
            }
            break;
        }
        fd += 1;
    }
    return -EAGAIN;
}

/**
 * @brief close
 */
static long sys_close(struct trapframe *tf)
{
    uint64_t fd = tf->gpr.a0;
    if (fd < 0 || fd > 4) return -EINVAL;
    struct vfs_inode *inode = current->fd[fd];
    current->fd[fd] = NULL;
    vfs_free_inode(inode);
    return 0;
}

/**
 * @brief stat
 */
static long sys_stat(struct trapframe *tf) {
    uint64_t fd = tf->gpr.a0;
    if (fd < 0 || fd > 4) return -EINVAL;
    struct vfs_inode *inode = current->fd[fd];
    if (!inode) return -EINVAL;
    struct vfs_stat *stat= vfs_get_stat(inode);
    memcpy((void *)tf->gpr.a1, stat, sizeof(struct vfs_stat));
    return 0;
}

/**
 * @brief read
 */
static long sys_read(struct trapframe *tf) {
    uint64_t fd = tf->gpr.a0;
    if (fd < 0 || fd > 4) return -EINVAL;
    struct vfs_inode *inode = current->fd[fd];
    if (!inode) return -EINVAL;
    vfs_inode_request(inode, (void *)tf->gpr.a1, tf->gpr.a2, 0, 1);
    return 0;
}

/**
 * @brief 关机、重启
 */
static long sys_reset(struct trapframe *tf)
{
    return reset_dev_test(tf->gpr.a0);
}

/**
 * @brief usleep 一段时间（微秒）
 */
static int64_t sys_usleep(struct trapframe *tf)
{
    return usleep_set((int64_t)tf->gpr.a0);
}

/**
 * @brief 系统调用表
 * 存储所有系统调用的指针的数组，系统调用号是其中的下标。
 * 所有系统调用都通过系统调用表调用
 */
fn_ptr syscall_table[] = {sys_init, sys_fork, sys_test_fork, sys_getpid, sys_getppid, sys_char, sys_block, sys_open, sys_close, sys_stat, sys_read, sys_reset, sys_usleep};

/**
 * @brief 通过系统调用号调用对应的系统调用
 *
 * @param number 系统调用号
 * @param ... 系统调用参数
 * @note 本实现中所有系统调用都仅在失败时返回负数，但实际上极小一部分 UNIX 系统调用（如
 *       `getpriority()`的正常返回值可能是负数的）。
 */
long syscall(long number, ...)
{
    va_list ap;
    va_start(ap, number);
    long arg1 = va_arg(ap, long);
    long arg2 = va_arg(ap, long);
    long arg3 = va_arg(ap, long);
    long arg4 = va_arg(ap, long);
    long arg5 = va_arg(ap, long);
    long arg6 = va_arg(ap, long);
    long ret = 0;
    va_end(ap);
    if (number > 0 && number < NR_TASKS) {
        /* 小心寄存器变量被覆盖 */
        register long a0 asm("a0") = arg1;
        register long a1 asm("a1") = arg2;
        register long a2 asm("a2") = arg3;
        register long a3 asm("a3") = arg4;
        register long a4 asm("a4") = arg5;
        register long a5 asm("a5") = arg6;
        register long a7 asm("a7") = number;
        __asm__ __volatile__ ("ecall\n\t"
                :"=r"(a0)
                :"r" (a1), "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a7)
                :"memory");
        ret = a0;
    } else {
        panic("Try to call unknown system call");
    }
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

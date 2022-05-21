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
    const char *path = (const char *)tf->gpr.a0;
    uint64_t flag = tf->gpr.a1;
    uint64_t mode = tf->gpr.a2;
    int64_t fd = vfs_user_get_free_fd(&current->vfs_context);
    if (fd < 0) return fd;
    int64_t ret = vfs_user_open(&current->vfs_context, path, flag, mode, fd);
    if (ret < 0) return ret;
    return fd;
}

/**
 * @brief close
 */
static long sys_close(struct trapframe *tf)
{
    uint64_t fd = tf->gpr.a0;
    return vfs_user_close(&current->vfs_context, fd);
}

/**
 * @brief stat
 */
static long sys_stat(struct trapframe *tf) {
    uint64_t fd = tf->gpr.a0;
    struct vfs_stat *stat= (struct vfs_stat *)tf->gpr.a1;
    return vfs_user_stat(&current->vfs_context, fd, stat);
}

/**
 * @brief lseek
 */
static long sys_seek(struct trapframe *tf) {
    uint64_t fd = tf->gpr.a0;
    int64_t offset = tf->gpr.a1;
    int64_t whence = tf->gpr.a2;
    return vfs_user_seek(&current->vfs_context, fd, offset, whence);
}

/**
 * @brief read
 */
static long sys_read(struct trapframe *tf) {
    uint64_t fd = tf->gpr.a0;
    char *buffer = (char *)tf->gpr.a1;
    uint64_t length = tf->gpr.a2;
    return vfs_user_read(&current->vfs_context, fd, length, buffer);
}

/**
 * @brief write
 */
static long sys_write(struct trapframe *tf) {
    uint64_t fd = tf->gpr.a0;
    char *buffer = (char *)tf->gpr.a1;
    uint64_t length = tf->gpr.a2;
    return vfs_user_write(&current->vfs_context, fd, length, buffer);
}

/**
 * @brief 关机、重启
 */
static long sys_reset(struct trapframe *tf)
{
    return reset_dev_test(tf->gpr.a0);
}

static long sys_test_net(struct trapframe *tf)
{
    return net_dev_test();
}

/**
 * @brief 系统调用表
 * 存储所有系统调用的指针的数组，系统调用号是其中的下标。
 * 所有系统调用都通过系统调用表调用
 */
fn_ptr syscall_table[] = {sys_init, sys_fork, sys_test_fork, sys_getpid, sys_getppid, sys_char, sys_block, sys_open, sys_close, sys_stat, sys_read, sys_write, sys_seek, sys_reset, sys_test_net};

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

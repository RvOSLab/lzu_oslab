#include <signal.h>
#include <sched.h>
#include <errno.h>
#include <string.h>

#define suser() (current->euid == 0) /* 检测是否为超级用户 */

// 向某进程（PCB）发送信号
static inline uint32_t send_sig(uint32_t signal, struct task_struct *p, uint32_t priv) // priv - 强制发送信号的标志。即不需要考虑进程用户属性或级别而能发送信号的权利
{
    if (!p)
        return -EINVAL;
    if (!priv && (current->euid != p->euid) && !suser()) // 如果不是 root 用户，并且不是当前进程的父进程
        return -EPERM;
    if ((signal == SIGKILL) || (signal == SIGCONT)) // 如果是杀死进程或者恢复进程信号，就恢复进程运行，并取消能停止进程的信号
    {
        if (p->state == TASK_STOPPED)
            p->state = TASK_RUNNING;
        p->exit_code = 0;
        p->signal &= ~((1 << SIGSTOP) | (1 << SIGTSTP) |
                       (1 << SIGTTIN) | (1 << SIGTTOU));
    }
    if (p->sigaction[signal].sa_handler == SIG_IGN) // 如果信号处理函数是 SIG_IGN，则直接忽略
        return 0;
    if ((signal >= SIGSTOP) && (signal <= SIGTTOU)) // 如果是 SIGSTOP, SIGTSTP, SIGTTIN, SIGTTOU 信号，需要停止进程，将 SIGCONT 信号取消防止进程继续
        p->signal &= ~(1 << (SIGCONT));
    if (signal == 0)
        return 0;
    p->signal |= (1 << signal); // 设置信号位

    do_signal(p, signal, NULL);
    return 0;
}

// 向某个 pid 发送信号
uint32_t kill_proc(uint32_t pid, uint32_t signal, uint32_t priv)
{
    if (signal < 1 || signal > NR_SIGNALS)
        return -EINVAL;
    for (uint32_t i = NR_TASKS - 1; i > -1; i--)
        if (tasks[i] && tasks[i]->pid == pid)
            return send_sig(signal, tasks[i], priv);
    return (-ESRCH);
}

// 向某进程组发信号，若进程组有一个进程被找到且成功发送，就返回 0，否则返回 -ESRCH 或发送信号时的错误
uint32_t kill_pg(uint32_t pgid, uint32_t signal, uint32_t priv)
{
    uint32_t err, retval = -ESRCH;
    uint32_t found = 0;

    for (uint32_t i = NR_TASKS - 1; i > -1; i--)
        if (tasks[i] && tasks[i]->pgid == pgid)
        {
            err = send_sig(signal, tasks[i], priv);
            if (err)
                retval = err;
            else
                found++;
        }
    return (found ? 0 : retval);
}

// 系统调用：各类的发送信号
uint32_t kill(uint32_t pid, uint32_t signal)
{
    if (signal < 1 || signal > NR_SIGNALS) // 提前检测信号是否合法
        return -EINVAL;
    if (!pid) // 如果 pid 为 0，则向所有同 pgid 进程发送信号
        return (kill_pg(current->pgid, signal, 0));
    if (pid == -1) // 如果 pid 为 -1，则向所有可送达信号的进程发送信号（普通用户是同 euid 进程，root 用户是所有进程）
    {
        uint32_t err, retval = 0;
        for (uint32_t i = NR_TASKS - 1; i > -1; i--)
            if ((err = send_sig(signal, tasks[i], 0)) != 0)
                retval = err;
        return retval;
    }
    if (pid < 0) // 如果 pid 为负数，则向所有 pgid == -PID 进程发送信号
        return (kill_pg(-pid, signal, 0));

    return (kill_proc(pid, signal, 0)); // 普通情况，向单个进程发送信号
}

// 设置信号处理函数
uint32_t set_sigaction(uint32_t signum, const struct sigaction *action, struct sigaction *oldaction)
{
    if (signum < 1 || signum > NR_SIGNALS || signum == SIGKILL || signum == SIGSTOP) // SIGKILL 与 SIGSTOP 不能被修改
        return -EINVAL;

    if (oldaction) // 在改动前先返回旧的结构体
        memcpy(oldaction, &(current->sigaction[signum]), sizeof(struct sigaction));

    memcpy(&(current->sigaction[signum]), action, sizeof(struct sigaction)); // 用新的结构体覆盖

    if (current->sigaction[signum].sa_flags & SA_NOMASK) // 如果设置了 SA_NOMASK，则清空信号屏蔽列表
        current->sigaction[signum].sa_mask = 0;
    else
        current->sigaction[signum].sa_mask |= (1 << signum); // 默认将当前信号加入到屏蔽列表中
    return 0;
}

uint32_t do_signal(struct task_struct *dest, uint32_t signr, struct trapframe *tf)
{
    struct sigaction sa = current->sigaction[signr];
    uint64_t sa_handler_old = (uint64_t)sa.sa_handler;

    if (sa_handler_old == (uint64_t)SIG_IGN) // 处理方法是忽略信号
    {
        /*  还没实现 sys_waitpid()，所以这里不能用
        if (signr == SIGCHLD) // 检查是否为 SIGCHLD
            while (sys_waitpid(-1, NULL, WNOHANG) > 0)
                ;
        */
        return 1; // 否则直接忽略
    }

    if (sa_handler_old == (uint64_t)SIG_DFL) // 若选择默认处理方法
    {
        switch (signr)
        {
        case SIGCONT:
        case SIGCHLD:
            return 1; // 直接忽略
        case SIGSTOP:
        case SIGTSTP:
        case SIGTTIN:
        case SIGTTOU:
            current->state = TASK_STOPPED;
            current->exit_code = signr;
            if (!(current->p_pptr->sigaction[SIGCHLD].sa_flags & SA_NOCLDSTOP))
                send_sig(SIGCHLD, current->p_pptr, 1);
            return 1;
        case SIGQUIT:
        case SIGILL:
        case SIGTRAP:
        case SIGIOT:
        case SIGFPE:
        case SIGSEGV:
        default:
            exit_process(dest->pid, 0);
        }
    }

    // 若为自定义处理函数
    if (sa.sa_flags & SA_ONESHOT) // 若为一次性的处理函数，执行后恢复默认处理
        sa.sa_handler = SIG_DFL;

    dest->sig_epc = dest->context.epc; // 复制信号处理之前目标进程的用户栈

    dest->context.epc = sa_handler_old;
    return 0;
}

int64_t sys_sigreturn(struct trapframe *tf){
    current->context.epc = current->sig_epc;
    return 0;
}

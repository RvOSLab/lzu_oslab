#ifndef __SIGNAL_H__
#define __SIGNAL_H__
#include <stddef.h>
#include <trap.h>
#include <sched.h>

#define NR_SIGNALS 22 /* 信号总数 */
#define SIGHUP 1      /* Hang Up	-- 挂起控制终端或进程 */
#define SIGINT 2      /* Interrupt -- 来自键盘的中断 */
#define SIGQUIT 3     /* Quit		-- 来自键盘的退出 */
#define SIGILL 4      /* Illeagle	-- 非法指令 */
#define SIGTRAP 5     /* Trap 	-- 跟踪断点 */
#define SIGABRT 6     /* Abort	-- 异常结束 */
#define SIGIOT 6      /* IO Trap	-- 同上 */
#define SIGUNUSED 7   /* Unused	-- 没有使用 */
#define SIGFPE 8      /* FPE		-- 协处理器出错 */
#define SIGKILL 9     /* Kill		-- 强迫进程终止 */
#define SIGUSR1 10    /* User1	-- 用户信号 1，进程可使用 */
#define SIGSEGV 11    /* Segment Violation -- 无效内存引用 */
#define SIGUSR2 12    /* User2    -- 用户信号 2，进程可使用 */
#define SIGPIPE 13    /* Pipe		-- 管道写出错，无读者 */
#define SIGALRM 14    /* Alarm	-- 实时定时器报警 */
#define SIGTERM 15    /* Terminate -- 进程终止 */
#define SIGSTKFLT 16  /* Stack Fault -- 栈出错（协处理器） */
#define SIGCHLD 17    /* Child	-- 子进程停止或被终止 */
#define SIGCONT 18    /* Continue	-- 恢复进程继续执行 */
#define SIGSTOP 19    /* Stop		-- 停止进程的执行 */
#define SIGTSTP 20    /* TTY Stop	-- tty 发出停止进程，可忽略 */
#define SIGTTIN 21    /* TTY In	-- 后台进程请求输入 */
#define SIGTTOU 22    /* TTY Out	-- 后台进程请求输出 */

// 一些 sa_flags 的定义
#define SA_NOCLDSTOP	1			/* 当子进程处于停止状态，就不对SIGCHLD处理 */
#define SA_INTERRUPT	0x20000000	/* 系统调用被信号中断后不重新启动系统调用 */
#define SA_NOMASK		0x40000000	/* 不阻止在指定的信号处理程序中再收到该信号 */
#define SA_ONESHOT		0x80000000 /* 信号句柄一旦被调用过就恢复到默认处理句柄 */

uint32_t kill(uint32_t pid, uint32_t signal);
uint32_t kill_pg(uint32_t pgid, uint32_t signal, uint32_t priv);
uint32_t kill_proc(uint32_t pid, uint32_t signal, uint32_t priv);
uint32_t set_sigaction(uint32_t signum, const struct sigaction *action, struct sigaction *oldaction);
uint32_t do_signal(struct task_struct *dest, uint32_t signr, struct trapframe *tf);
uint64_t sys_sigreturn(struct trapframe *tf);

// 使用不可能被调用的地址来充当特殊信号处理程序
#define SIG_DFL ((void (*)(uint32_t))0) /* default signal handling */     /* 默认信号处理程序（信号句柄） */
#define SIG_IGN ((void (*)(uint32_t))1) /* ignore signal */               /* 忽略信号的处理程序 */
#define SIG_ERR ((void (*)(uint32_t)) - 1) /* error return from signal */ /* 信号处理返回错误 */

#endif
